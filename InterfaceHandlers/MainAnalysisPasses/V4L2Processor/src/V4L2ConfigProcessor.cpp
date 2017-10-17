//
// Created by machiry on 5/8/17.
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
#include "IOInstVisitor.h"
#include "llvm/IR/DebugInfoMetadata.h"


using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {

    static cl::opt<std::string> v4l2_func_config_file("v4l2config",
                                                      cl::desc("v4l2 Function config file."),
                                                      cl::value_desc("full path to the v4l2 config file"), cl::init(""));

    static cl::opt<std::string> v4l2_cmd_config_file("v4l2idconfig",
                                                     cl::desc("Path to the output file where function index to cmdid should be stored."),
                                                     cl::value_desc("full path to the output v4l2 function index to cmdid"), cl::init(""));

    struct V4L2ConfigProcessor: public ModulePass {
    public:
        static char ID;
        //GlobalState moduleState;

        V4L2ConfigProcessor() : ModulePass(ID) {
        }

        ~V4L2ConfigProcessor() {
        }

        void readV4L2Config(std::string &v4l2_config_file, std::map<std::string, unsigned> &v4L2Config) {
            FILE *fp = fopen(v4l2_config_file.c_str(), "r");
            unsigned currindex;
            char funcName[1024];
            char currLine[1024];
            unsigned long cIn;
            while(fscanf(fp, "%s\n", currLine) != EOF) {
                std::string currStr(currLine);
                cIn = currStr.find(',', 0);
                std::string currFName = currStr.substr(0, cIn);
                std::string funcIdStr = currStr.substr(cIn+1);
                sscanf(funcIdStr.c_str(), "%u", &currindex);
                v4L2Config[currFName] = currindex;
            }
            fclose(fp);
        }

        std::string handleCustomization(std::string &providedStr) {
            if(providedStr == "vidioc_enuminput") {
                return "vidioc_enum_input";
            }
            if(providedStr == "vidioc_enumoutput") {
                return "vidioc_enum_output";
            }
            if(providedStr == "vidioc_g_chip_info") {
                return "vidioc_g_chip_ident";
            }
            return providedStr;
        }

        void mapMultiple(std::string &funcName, uint64_t targetCmd, std::map<std::string, unsigned> &v4L2Config,
                         std::map<uint64_t , uint64_t> &id_to_cmdid) {
            // iterate thru c++ map
            for(auto a: v4L2Config) {
                const std::string &curr_guy = a.first;
                if(curr_guy.find(funcName) == 0) {
                    id_to_cmdid[v4L2Config[curr_guy]] = targetCmd;
                }
            }
        }

        bool runOnModule(Module &m) override {
            Module::GlobalListType &currGlobalList = m.getGlobalList();
            std::vector<StructType*> allStTypes = m.getIdentifiedStructTypes();
            StructType *targetStType = nullptr;
            const StructLayout *targetStLayout = nullptr;
            std::map<std::string, unsigned> v4L2Config;
            readV4L2Config(v4l2_func_config_file, v4L2Config);
            DataLayout *currDataLayout = new DataLayout(&m);
            std::map<uint64_t , uint64_t> id_to_cmdid;
            for(auto a:allStTypes) {
                if(a->hasName() && a->getName() == "struct.v4l2_ioctl_ops") {
                    targetStType = a;
                    targetStLayout = currDataLayout->getStructLayout(targetStType);

                }
            }

            for (Module::global_iterator gstart = currGlobalList.begin(), gend = currGlobalList.end();
                 gstart != gend; gstart++) {
                // ignore constant immutable global pointers
                if ((*gstart).isConstant()) {
                    continue;
                }

                llvm::GlobalVariable *globalVariable = &(*gstart);

                std::string curr_global_name = globalVariable->getName();
                if(curr_global_name == "v4l2_ioctls") {
                    // OK, we got our guy
                    if (globalVariable->hasInitializer()) {
                        Constant *baseInitializer = globalVariable->getInitializer();

                        Type *initType = baseInitializer->getType();
                        //dbgs() << "Init:" << *baseInitializer << "\n";
                        //dbgs() << "Type:" << *initType << "\n";
                        //assert(initType->isStructTy());


                        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(baseInitializer);
                        if(actualStType != nullptr) {
                            for (unsigned int i = 0; i < actualStType->getNumOperands(); i++) {
                                Value *currFieldVal = actualStType->getOperand(i);
                                Constant *constCheck = dyn_cast<Constant>(currFieldVal);
                                Type *currValType = constCheck->getType();
                                assert(currValType->isStructTy());
                                StructType *valStType = dyn_cast<StructType>(currValType);
                                if(valStType->hasName()) {
                                    //dbgs() << valStType->getName() << "\n";
                                    assert(valStType->getName() == "struct.v4l2_ioctl_info");
                                }
                                ConstantStruct *structVal = dyn_cast<ConstantStruct>(currFieldVal);
                                if(structVal != nullptr) {
                                    ConstantInt *cmdId = dyn_cast<ConstantInt>(structVal->getOperand(0));
                                    //dbgs() << cmdId->getZExtValue() << "\n";
                                    ConstantStruct *cmdFunc = dyn_cast<ConstantStruct>(structVal->getOperand(3));
                                    //dbgs() << *cmdFunc << "\n";
                                    Constant *targetInit = cmdFunc->getOperand(0);
                                    Function *targetFunc = dyn_cast<Function>(targetInit);
                                    if(targetFunc != nullptr) {
                                        //dbgs() << "Target Func:" << targetFunc->getName() << "\n";
                                        string targetFuncName = targetFunc->getName();
                                        string finalFun = "fail";
                                        if(targetFuncName.find("v4l_dbg") == 0) {
                                            finalFun = "vidioc" + targetFuncName.substr(7, targetFuncName.length()-7);
                                        } else {
                                            if(targetFuncName.find("v4l") == 0) {
                                                finalFun = "vidioc" + targetFuncName.substr(3, targetFuncName.length()-3);
                                            }
                                        }
                                        //dbgs() << "Target Func:" << finalFun << "\n";
                                        if(v4L2Config.find(finalFun) != v4L2Config.end()) {
                                            unsigned cmdIndex = v4L2Config[finalFun];
                                            id_to_cmdid[(uint64_t)cmdIndex] = cmdId->getZExtValue();
                                        } else {
                                            std::string modFunc = handleCustomization(finalFun);
                                            if(modFunc != finalFun) {
                                                unsigned cmdIndex = v4L2Config[modFunc];
                                                id_to_cmdid[(uint64_t)cmdIndex] = cmdId->getZExtValue();
                                            } else {
                                                mapMultiple(finalFun, cmdId->getZExtValue(), v4L2Config, id_to_cmdid);
                                            }
                                            /*dbgs() << "[-] Unable to find function:" << finalFun
                                                   << " in the provided config:" << v4l2_func_config_file << "\n";*/
                                        }
                                    } else {
                                        ConstantInt *targetOff = dyn_cast<ConstantInt>(targetInit);
                                        unsigned eleIndex = targetStLayout->getElementContainingOffset(targetOff->getZExtValue());
                                        id_to_cmdid[(uint64_t)eleIndex] = cmdId->getZExtValue();


                                        //dbgs() << "Target offset:" << eleIndex << "\n";
                                        //dbgs() << "ActualElem:" <<  *(targetStType->getElementType(eleIndex)) << "\n";
                                    }
                                }

                            }
                        }
                    }
                }
            }
            dbgs() << "[+] Writing all id to cmds to file:" << v4l2_cmd_config_file << "\n";
            FILE *outputfp = fopen(v4l2_cmd_config_file.c_str(), "w");
            for(auto curra:id_to_cmdid) {
                fprintf(outputfp, "%ld,%ld\n", curra.first, curra.second);
            }
            fclose(outputfp);
        }



        void getAnalysisUsage(AnalysisUsage &AU) const override {
            AU.setPreservesAll();
            //AU.addRequired<InterProceduralRA<CropDFS>>();
            AU.addRequired<CallGraphWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
        }

    };

    char V4L2ConfigProcessor::ID = 0;
    static RegisterPass<V4L2ConfigProcessor> x("v4l2-config-processor", "V4L2 Config Processor", false, true);
}