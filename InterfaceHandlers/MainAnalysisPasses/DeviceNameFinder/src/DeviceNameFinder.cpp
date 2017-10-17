//
// Created by machiry on 4/26/17.
//

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ValueSymbolTable.h"
#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/Module.h"
//#include "RangePass.h"
//#include "RangeAnalysis.h"
#include "CFGUtils.h"
#include "FileUtils.h"
//#include "IOInstVisitor.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "TypePrintHelper.h"
#include <llvm/IR/Operator.h>


using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {


    static cl::opt<std::string> checkFunctionName("ioctlFunction",
                                                  cl::desc("Function whose device name need to be identified."),
                                                  cl::value_desc("full name of the function"), cl::init(""));


    struct DeviceNameFinderPass: public ModulePass {
    public:
        static char ID;
        //GlobalState moduleState;

        DeviceNameFinderPass() : ModulePass(ID) {
        }

        ~DeviceNameFinderPass() {
        }

        bool isCalledFunction(CallInst *callInst, std::string &funcName) {
            Function *targetFunction = nullptr;
            if(callInst != nullptr) {
                targetFunction = callInst->getCalledFunction();
                if(targetFunction == nullptr) {
                   targetFunction = dyn_cast<Function>(callInst->getCalledValue()->stripPointerCasts());
                }
            }
            return (targetFunction != nullptr && targetFunction->hasName() &&
                    targetFunction->getName() == funcName);
        }

        llvm::GlobalVariable *getSrcTrackedVal(Value *currVal) {
            llvm::GlobalVariable *currGlob = dyn_cast<llvm::GlobalVariable>(currVal);
            if(currGlob == nullptr) {
                Instruction *currInst = dyn_cast<Instruction>(currVal);
                if(currInst != nullptr) {
                    for(unsigned int i=0; i < currInst->getNumOperands(); i++) {
                        Value *currOp = currInst->getOperand(i);
                        llvm::GlobalVariable *opGlobVal = getSrcTrackedVal(currOp);
                        if(opGlobVal == nullptr) {
                            opGlobVal = getSrcTrackedVal(currOp->stripPointerCasts());
                        }
                        if(opGlobVal != nullptr) {
                            currGlob = opGlobVal;
                            break;
                        }
                    }
                } else {
                    return nullptr;
                }
            }
            return currGlob;
        }

        llvm::AllocaInst *getSrcTrackedAllocaVal(Value *currVal) {
            //dbgs() <<"dada:" << *currVal << "\n";
            llvm::AllocaInst *currGlob = dyn_cast<AllocaInst>(currVal);
            if(currGlob == nullptr) {
                Instruction *currInst = dyn_cast<Instruction>(currVal);
                if(currInst != nullptr) {
                    for(unsigned int i=0; i < currInst->getNumOperands(); i++) {
                        Value *currOp = currInst->getOperand(i);
                        llvm::AllocaInst *opGlobVal = getSrcTrackedAllocaVal(currOp);
                        if(opGlobVal == nullptr) {
                            opGlobVal = getSrcTrackedAllocaVal(currOp->stripPointerCasts());
                        }
                        if(opGlobVal != nullptr) {
                            currGlob = opGlobVal;
                            break;
                        }
                    }
                } else {
                    return nullptr;
                }
            }
            return currGlob;
        }

        llvm::Value *getSrcTrackedArgVal(Value *currVal, Function *targetFunction) {
            //dbgs() <<"dada:" << *currVal << "\n";
            Instruction *currInst = dyn_cast<Instruction>(currVal);
            if (currInst == nullptr) {
                for (auto &a:targetFunction->getArgumentList()) {
                    if (&a == currVal) {
                        return &a;
                    }
                }
            } else {

                for (unsigned int i = 0; i < currInst->getNumOperands(); i++) {
                    Value *currOp = currInst->getOperand(i);
                    llvm::Value *opGlobVal = getSrcTrackedArgVal(currOp, targetFunction);
                    if (opGlobVal == nullptr) {
                        opGlobVal = getSrcTrackedArgVal(currOp->stripPointerCasts(), targetFunction);
                    }
                    if (opGlobVal != nullptr) {
                        return opGlobVal;
                    }
                }
            }
            return nullptr;

        }

        bool getDeviceString(Value *currVal) {
            const GEPOperator *gep = dyn_cast<GEPOperator>(currVal);
            const llvm::GlobalVariable *strGlobal = nullptr;
            if(gep != nullptr) {
                 strGlobal = dyn_cast<GlobalVariable>(gep->getPointerOperand());

            }
            if (strGlobal != nullptr && strGlobal->hasInitializer()) {
                const Constant *currConst = strGlobal->getInitializer();
                const ConstantDataArray *currDArray = dyn_cast<ConstantDataArray>(currConst);
                if(currDArray != nullptr) {
                    dbgs() << "[+] Device Name: " << currDArray->getAsCString() << "\n";
                } else {
                    dbgs() << "[+] Device Name: " << *currConst << "\n";
                }
                return true;
            }
            return false;
        }

        CallInst *getRecursiveCallInst(Value *srcVal, std::string &targetFuncName, std::set<Value*> &visitedVals) {
            CallInst *currInst = nullptr;
            if(visitedVals.find(srcVal) != visitedVals.end()) {
                return nullptr;

            }
            //dbgs() <<  "Getting Rec:" << targetFuncName << "\n";
            visitedVals.insert(srcVal);
            for(auto curr_use:srcVal->users()) {
                currInst = dyn_cast<CallInst>(curr_use);
                if(currInst && isCalledFunction(currInst, targetFuncName)) {
                    break;
                }
                currInst = nullptr;
            }
            if(currInst == nullptr) {
                for(auto curr_use:srcVal->users()) {
                    currInst = getRecursiveCallInst(curr_use, targetFuncName, visitedVals);
                    if(currInst && isCalledFunction(currInst, targetFuncName)) {
                        break;
                    }
                    currInst = nullptr;
                }
            }
            visitedVals.erase(visitedVals.find(srcVal));
            return currInst;
        }

        bool handleGenericCDevRegister(Function *currFunction) {
            for(Function::iterator fi = currFunction->begin(), fe = currFunction->end(); fi != fe; fi++) {
                BasicBlock &currBB = *fi;
                for (BasicBlock::iterator i = currBB.begin(), ie = currBB.end(); i != ie; ++i) {
                    Instruction *currInstPtr = &(*i);
                    CallInst *currCall = dyn_cast<CallInst>(currInstPtr);

                    std::string allocChrDev = "alloc_chrdev_region";
                    if(isCalledFunction(currCall, allocChrDev)) {
                        Value *devNameOp = currCall->getArgOperand(3);
                        return getDeviceString(devNameOp);
                    }
                    // is the usage device_create?
                    std::string devCreateName = "device_create";
                    if(isCalledFunction(currCall, devCreateName)) {
                        Value *devNameOp = currCall->getArgOperand(4);
                        return getDeviceString(devNameOp);
                    }

                    std::string regChrDevRegion = "register_chrdev_region";
                    if(isCalledFunction(currCall, regChrDevRegion)) {
                        Value *devNameOp = currCall->getArgOperand(2);
                        return getDeviceString(devNameOp);
                    }
                }
            }
            return false;
        }

        bool handleCdevHeuristic(CallInst *cdevCallInst, llvm::GlobalVariable *fopsStructure) {
            Value *deviceStructure = cdevCallInst->getArgOperand(0);
            // get the dev structure
            //llvm::GlobalVariable *globalDevSt = dyn_cast<llvm::GlobalVariable>(deviceStructure);
            llvm::GlobalVariable *globalDevSt = getSrcTrackedVal(deviceStructure);
            if(globalDevSt != nullptr) {
                // globalDevSt->dump();
                CallInst *cdev_call= nullptr;

                // get the cdev_add function which is using the dev structure.
                /*for(auto curr_use:globalDevSt->users()) {
                    cdev_call = dyn_cast<CallInst>(curr_use);
                    if(isCalledFunction(cdev_call, "cdev_add")) {
                        break;
                    }
                }*/
                std::set<Value*> visitedVals;
                visitedVals.clear();
                std::string cdevaddFunc = "cdev_add";
                cdev_call = getRecursiveCallInst(globalDevSt, cdevaddFunc, visitedVals);
                if(cdev_call != nullptr) {
                    // find, the devt structure.
                    llvm::GlobalVariable *target_devt = getSrcTrackedVal(cdev_call->getArgOperand(1));

                    llvm::AllocaInst *allocaVal = nullptr;
                    Value *targetDevNo;
                    targetDevNo = target_devt;

                    if(target_devt == nullptr) {
                        allocaVal = getSrcTrackedAllocaVal(cdev_call->getArgOperand(1));
                        targetDevNo = allocaVal;
                    }

                    if(targetDevNo == nullptr) {
                        targetDevNo = getSrcTrackedArgVal(cdev_call->getArgOperand(1), cdevCallInst->getFunction());
                    }

                    // find the uses.
                    if(targetDevNo != nullptr ) {
                        /*for(auto curr_use:target_devt->users()) {
                            CallInst *currCall = dyn_cast<CallInst>(curr_use);
                            // is the usage alloc_chrdev?
                            if(isCalledFunction(currCall, "alloc_chrdev_region")) {
                                Value *devNameOp = currCall->getArgOperand(3);
                                return getDeviceString(devNameOp);
                            }
                            // is the usage device_create?
                            if(isCalledFunction(currCall, "device_create")) {
                                Value *devNameOp = currCall->getArgOperand(4);
                                return getDeviceString(devNameOp);
                            }
                        }*/
                        visitedVals.clear();
                        // dbgs() <<  "DevNo:" << *targetDevNo << "\n";
                        std::string allocChrDevReg = "alloc_chrdev_region";
                        CallInst *currCall = getRecursiveCallInst(targetDevNo, allocChrDevReg, visitedVals);
                        if(currCall != nullptr) {
                            Value *devNameOp = currCall->getArgOperand(3);
                            return getDeviceString(devNameOp);
                        }
                        visitedVals.clear();
                        std::string devCreate = "device_create";
                        currCall = getRecursiveCallInst(targetDevNo, devCreate, visitedVals);
                        // is the usage device_create?
                        if(currCall) {
                            Value *devNameOp = currCall->getArgOperand(4);
                            return getDeviceString(devNameOp);
                        }

                    }
                }
                return handleGenericCDevRegister(cdevCallInst->getParent()->getParent());
            }
            return false;
        }

        bool handleProcCreateHeuristic(CallInst *procCallInst, llvm::GlobalVariable *fopsStructure) {
            Value *devNameOp = procCallInst->getArgOperand(0);
            return getDeviceString(devNameOp);
        }

        bool handleMiscDevice(llvm::GlobalVariable *miscDevice) {
            if(miscDevice->hasInitializer()) {
                ConstantStruct *outputSt = dyn_cast<ConstantStruct>(miscDevice->getInitializer());
                if(outputSt != nullptr) {
                    Value *devNameVal = outputSt->getOperand(1);
                    return getDeviceString(devNameVal);
                }
            }
            return false;
        }


        bool handleDynamicMiscOps(StoreInst *srcStoreInst) {
#define MISC_FILENAME_INDX 1
            // OK this is the store instruction which is trying to store fops into a misc device
            BasicBlock *targetBB = srcStoreInst->getParent();
            // iterate thru all the instructions to find any store to a misc device name field.
            std::set<Value *> nameField;
            for (BasicBlock::iterator i = targetBB->begin(), ie = targetBB->end(); i != ie; ++i) {
                Instruction *currInstPtr = &(*i);
                GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(currInstPtr);
                if(gepInst && gepInst->getNumOperands() > 2) {
                    //dbgs() << *(gepInst->getSourceElementType()) << "\n";
                    StructType *targetStruct = dyn_cast<StructType>(gepInst->getSourceElementType());
                    //dbgs() << targetStruct->getName() << "\n";
                    if (targetStruct != nullptr && targetStruct->getName() == "struct.miscdevice") {
                        //dbgs() <<  "Found:" << *gepInst << "\n";
                        ConstantInt *fieldInt = dyn_cast<ConstantInt>(gepInst->getOperand(2));
                        if(fieldInt) {
                            if(fieldInt->getZExtValue() == MISC_FILENAME_INDX) {
                                nameField.insert(&(*i));
                            }
                        }
                    }
                }

                // Are we storing into name field of a misc structure?
                StoreInst *currStInst = dyn_cast<StoreInst>(currInstPtr);
                if(currStInst) {
                    Value *targetPtr = currStInst->getPointerOperand()->stripPointerCasts();
                    if(nameField.find(targetPtr) != nameField.end()) {
                        // YES.
                        // find the name
                        return getDeviceString(currStInst->getOperand(0));
                    }
                }
            }
            return false;
        }

        void handleV4L2Dev(Module &m) {
#define VFL_TYPE_GRABBER	0
#define VFL_TYPE_VBI		1
#define VFL_TYPE_RADIO		2
#define VFL_TYPE_SUBDEV		3
#define VFL_TYPE_SDR		4
#define VFL_TYPE_TOUCH		5

            std::map<uint64_t, std::string> v4l2TypeNameMap;
            v4l2TypeNameMap[VFL_TYPE_GRABBER] = "/dev/video[X]";
            v4l2TypeNameMap[VFL_TYPE_VBI] = "/dev/vbi[X]";
            v4l2TypeNameMap[VFL_TYPE_RADIO] = "/dev/radio[X]";
            v4l2TypeNameMap[VFL_TYPE_SUBDEV] = "/dev/subdev[X]";
            v4l2TypeNameMap[VFL_TYPE_SDR] = "/dev/swradio[X]";
            v4l2TypeNameMap[VFL_TYPE_TOUCH] = "/dev/v4l-touch[X]";

            // find all functions
            for(Module::iterator mi = m.begin(), ei = m.end(); mi != ei; mi++) {
                Function &currFunction = *mi;
                if(!currFunction.isDeclaration() && currFunction.hasName()) {
                    string currFuncName = currFunction.getName();
                    if(currFuncName.find("init") != string::npos || currFuncName.find("probe") != string::npos) {
                        for(Function::iterator fi = currFunction.begin(), fe = currFunction.end(); fi != fe; fi++) {
                            BasicBlock &currBB = *fi;
                            for (BasicBlock::iterator i = currBB.begin(), ie = currBB.end(); i != ie; ++i) {
                                Instruction *currInstPtr = &(*i);
                                CallInst *currCall = dyn_cast<CallInst>(currInstPtr);
                                if(currCall != nullptr) {
                                    Function *calledFunc = currCall->getCalledFunction();
                                    if (calledFunc != nullptr && calledFunc->hasName() &&
                                        calledFunc->getName() == "__video_register_device") {
                                        Value *devType = currCall->getArgOperand(1);
                                        //InterProceduralRA<CropDFS> &range_analysis = getAnalysis<InterProceduralRA<CropDFS>>();
                                        //Range devRange = range_analysis.getRange(devType);
                                        ConstantInt *cInt = dyn_cast<ConstantInt>(devType);
                                        if (cInt) {
                                            uint64_t typeNum = cInt->getZExtValue();
                                            if (v4l2TypeNameMap.find(typeNum) != v4l2TypeNameMap.end()) {
                                                dbgs() << "[+] V4L2 Device: " << v4l2TypeNameMap[typeNum] << "\n";
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }


        bool runOnModule(Module &m) override {
            dbgs() << "[+] Provided Function Name: " << checkFunctionName << "\n";

            for(Module::iterator mi = m.begin(), ei = m.end(); mi != ei; mi++) {

                Function &currFunction = *mi;
                //if the current function is the target function.
                if(!currFunction.isDeclaration() && currFunction.hasName() &&
                   currFunction.getName().str() == checkFunctionName) {

                    llvm::GlobalVariable *targetFopsStructure = nullptr;
                    for(auto curr_use: currFunction.users()) {
                        if(curr_use->getType()->isStructTy()) {
                            for(auto curr_use1: curr_use->users()) {
                                llvm::GlobalVariable *currGlobal = dyn_cast<llvm::GlobalVariable>(curr_use1);
                                if(currGlobal != nullptr) {
                                    targetFopsStructure = currGlobal;
                                    dbgs() << "[+] Found Fops Structure: " << targetFopsStructure->getName() << "\n";
                                    break;

                                }
                            }
                            if(targetFopsStructure != nullptr) {
                                break;
                            }
                        }
                    }

                    if(targetFopsStructure == nullptr) {
                        dbgs() << "[-] Unable to find fops structure which is using this function.\n";
                    } else {
                        for(auto curr_usage:targetFopsStructure->users()) {
                            CallInst *currCallInst = dyn_cast<CallInst>(curr_usage);
                            std::string cdevInitName = "cdev_init";
                            if(isCalledFunction(currCallInst, cdevInitName)) {
                                if(handleCdevHeuristic(currCallInst, targetFopsStructure)) {
                                    dbgs() << "[+] Device Type: char\n";
                                    dbgs() << "[+] Found using first cdev heuristic\n";
                                }

                            }

                            std::string regChrDev = "__register_chrdev";
                            if(isCalledFunction(currCallInst, regChrDev)) {
                                if(getDeviceString(currCallInst->getArgOperand(3))) {
                                    dbgs() << "[+] Device Type: char\n";
                                    dbgs() << "[+] Found using register chrdev heuristic\n";
                                }

                            }


                            std::string procCreateData = "proc_create_data";
                            std::string procCreateName = "proc_create";
                            if(isCalledFunction(currCallInst, procCreateData) ||
                                    isCalledFunction(currCallInst, procCreateName)) {
                                if(handleProcCreateHeuristic(currCallInst, targetFopsStructure)) {
                                    dbgs() << "[+] Device Type: proc\n";
                                    dbgs() << "[+] Found using proc create heuristic\n";
                                }
                            }


                            // Handling misc devices
                            // Standard. (when misc device is defined as static)
                            if(curr_usage->getType()->isStructTy()) {
                                llvm::GlobalVariable *currGlobal = nullptr;
                                // OK, find the miscdevice
                                for(auto curr_use1: curr_usage->users()) {
                                    currGlobal = dyn_cast<llvm::GlobalVariable>(curr_use1);
                                    if(currGlobal != nullptr) {
                                        break;
                                    }
                                }
                                if(currGlobal != nullptr) {
                                    for (auto second_usage:currGlobal->users()) {

                                        CallInst *currCallInst = dyn_cast<CallInst>(second_usage);
                                        std::string miscRegFunc = "misc_register";
                                        if (isCalledFunction(currCallInst, miscRegFunc)) {
                                            if (handleMiscDevice(currGlobal)) {
                                                dbgs() << "[+] Device Type: misc\n";
                                                dbgs() << "[+] Found using misc heuristic\n";
                                            }

                                        }
                                    }
                                }
                            }

                            // when misc device is created manually
                            // using kmalloc
                            StoreInst *currStore = dyn_cast<StoreInst>(curr_usage);
                            if(currStore != nullptr) {
                                // OK, we are storing into a structure.
                                if(handleDynamicMiscOps(currStore)) {
                                    dbgs() << "[+] Device Type: misc\n";
                                    dbgs() << "[+] Found using dynamic misc heuristic\n";
                                }
                            }

                            // Handling v4l2 devices
                            // More information: https://01.org/linuxgraphics/gfx-docs/drm/media/kapi/v4l2-dev.html
                            Type *targetFopsType = targetFopsStructure->getType()->getContainedType(0);
                            if(targetFopsType->isStructTy()) {
                                StructType *fopsStructType = dyn_cast<StructType>(targetFopsType);
                                if(fopsStructType->hasName()) {
                                    std::string structureName = fopsStructType->getStructName();
                                    if(structureName == "struct.v4l2_ioctl_ops") {
                                        handleV4L2Dev(m);
                                        dbgs() << "[+] Device Type: v4l2\n";
                                        dbgs() << "[+] Look into: /sys/class/video4linux/<devname>/name to know the details\n";
                                        return false;
                                    }
                                }
                            }

                        }
                    }
                }
            }
            return false;
        }



        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.setPreservesAll();
            //AU.addRequired<InterProceduralRA<CropDFS>>();
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            //AU.addRequired<unimelb::WrappedRangePass>();
        }

    private:



    };

    char DeviceNameFinderPass::ID = 0;
    static RegisterPass<DeviceNameFinderPass> x("dev-name-finder", "Device name finder", false, true);
}