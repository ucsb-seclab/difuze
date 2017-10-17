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
#include "IOInstVisitor.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "TypePrintHelper.h"
#include "DefIoctlInstVisitor.h"


using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {


    static cl::opt<std::string> checkFunctionName("ioctlFunction",
                                                  cl::desc("v4l2 Function which is to be considered as entry point "
                                                                   "into the driver"),
                                                  cl::value_desc("full name of the function"), cl::init(""));

    static cl::opt<int> funcIndexInStruct("funcIndex", cl::desc("Index of the function in the v4l2_ioctl_ops structure"),
                                          cl::value_desc(""), cl::init(-1));

    static cl::opt<std::string> v4l2_index_config("v4l2indexconfig",
                                                 cl::desc("Path to the output file where function index to cmd id are stored."),
                                                 cl::value_desc("full path to the v4l2 function index to cmdid"), cl::init(""));

    static cl::opt<std::string> bitcodeOutDir("bcOutDir",
                                              cl::desc("Base path where LLVM output is produced."),
                                              cl::value_desc("Absolute path to the directory "
                                                                     "containing the complete bitcode."),
                                              cl::init(""));

    static cl::opt<std::string> srcBaseDir("srcBaseDir",
                                           cl::desc("Base path of the kernel sources."),
                                           cl::value_desc("Absolute path to the directory "
                                                                  "containing the linux source code."),
                                           cl::init(""));


    struct IoctlCmdCheckerPass: public ModulePass {
    public:
        static char ID;
        //GlobalState moduleState;

        IoctlCmdCheckerPass() : ModulePass(ID) {
        }

        ~IoctlCmdCheckerPass() {
        }

        void readV4L2Config(std::string &v4l2_config_file, std::map<unsigned, unsigned> &v4L2Config) {
            FILE *fp = fopen(v4l2_config_file.c_str(), "r");
            unsigned currindex;
            unsigned cmdId;
            char funcName[1024];
            char currLine[1024];
            unsigned long cIn;
            while(fscanf(fp, "%s\n", currLine) != EOF) {
                std::string currStr(currLine);
                cIn = currStr.find(',', 0);
                std::string currFName = currStr.substr(0, cIn);
                std::string funcIdStr = currStr.substr(cIn+1);
                sscanf(currFName.c_str(), "%u", &currindex);
                sscanf(funcIdStr.c_str(), "%u", &cmdId);
                v4L2Config[currindex] = cmdId;
            }
            fclose(fp);
        }

        bool runOnModule(Module &m) override {
            dbgs() << "Provided Function Name:" << checkFunctionName << "\n";
            if(funcIndexInStruct == -1) {
                dbgs() << "You should provide functionIndex.\n";
                assert(false);
            }
            std::map<unsigned, unsigned> v4L2Config;
            // read the id to index config.
            readV4L2Config(v4l2_index_config, v4L2Config);
            // make sure that index in struct is present
            if(v4L2Config.find((unsigned)funcIndexInStruct) != v4L2Config.end()) {
                //dbgs() << "CommandID:" << v4L2Config[funcIndexInStruct] << "\n";

                std::set<string> all_c_preprocessed_files;
                SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
                std::string targetFName, tmpFilePath;
                std::string includePrefix = ".includes";
                std::string preprocessedPrefix = ".preprocessed";

                for (Module::iterator mi = m.begin(), ei = m.end(); mi != ei; mi++) {

                    Function &currFunction = *mi;

                    currFunction.getAllMetadata(MDs);
                    for (auto &MD : MDs) {
                        if (MDNode *N = MD.second) {
                            if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                                targetFName = subProgram->getFilename();
                                tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir, preprocessedPrefix);
                                if(all_c_preprocessed_files.find(tmpFilePath) == all_c_preprocessed_files.end()) {
                                    all_c_preprocessed_files.insert(tmpFilePath);
                                }
                                break;
                            }
                        }
                    }

                    //if the current function is the target function.
                    if (!currFunction.isDeclaration() && currFunction.hasName() &&
                        currFunction.getName().str() == checkFunctionName) {
                        currFunction.getAllMetadata(MDs);
                        TypePrintHelper::setFoldersPath(srcBaseDir, bitcodeOutDir);
                        //dbgs() << "Accepting Type:\n";
                        Type *lastArgType = currFunction.getFunctionType()->getParamType(
                                currFunction.getFunctionType()->getNumParams() - 1);
                        dbgs() << "Found Cmd:" << v4L2Config[funcIndexInStruct] << ":START\n";
                        TypePrintHelper::printType(lastArgType, dbgs());
                        dbgs() << "Found Cmd:" << v4L2Config[funcIndexInStruct] << ":END\n";

                        for (auto &MD : MDs) {
                            if (MDNode *N = MD.second) {
                                if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                                    targetFName = subProgram->getFilename();
                                    tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir,
                                                                                preprocessedPrefix);
                                    if (TypePrintHelper::requiredPreprocessingFiles.find(tmpFilePath) ==
                                        TypePrintHelper::requiredPreprocessingFiles.end()) {
                                        TypePrintHelper::requiredPreprocessingFiles.insert(tmpFilePath);
                                    }
                                    tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir,
                                                                                includePrefix);
                                    if (TypePrintHelper::requiredIncludeFiles.find(tmpFilePath) ==
                                        TypePrintHelper::requiredIncludeFiles.end()) {
                                        TypePrintHelper::requiredIncludeFiles.insert(tmpFilePath);
                                    }
                                    break;
                                }
                            }
                        }


                        //InterProceduralRA<CropDFS> &range_analysis = getAnalysis<InterProceduralRA<CropDFS>>();
                        std::set<int> cArg;
                        std::set<int> uArg;
                        std::vector<Value *> callStack;
                        std::map<unsigned, Value *> callerArguments;
                        cArg.insert(1);
                        uArg.insert(2);
                        callerArguments.clear();
                        IOInstVisitor *currFuncVis = new IOInstVisitor(&currFunction, currFunction.getFunctionType()->getNumParams() - 1);
                        // start analyzing
                        currFuncVis->analyze();
                        for (auto a:TypePrintHelper::requiredIncludeFiles) {
                            dbgs() << "Includes file:" << a << "\n";
                        }
                        for (auto a:TypePrintHelper::requiredPreprocessingFiles) {
                            dbgs() << "Preprocessed file:" << a << "\n";
                        }
                        dbgs() << "ALL PREPROCESSED FILES:\n";
                        for(auto a:all_c_preprocessed_files) {
                            dbgs() << "Compl Preprocessed file:" << a << "\n";
                        }
                    }
                }
            } else {
                dbgs() << "Custom V4L2 IOCTL\n";
                // This is the default ioctl handler..process it as regular ioctl cmd
                std::set<string> all_c_preprocessed_files;
                SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
                std::string targetFName, tmpFilePath;
                std::string includePrefix = ".includes";
                std::string preprocessedPrefix = ".preprocessed";

                for (Module::iterator mi = m.begin(), ei = m.end(); mi != ei; mi++) {

                    Function &currFunction = *mi;
                    currFunction.getAllMetadata(MDs);
                    for (auto &MD : MDs) {
                        if (MDNode *N = MD.second) {
                            if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                                targetFName = subProgram->getFilename();
                                tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir, preprocessedPrefix);
                                if(all_c_preprocessed_files.find(tmpFilePath) == all_c_preprocessed_files.end()) {
                                    all_c_preprocessed_files.insert(tmpFilePath);
                                }
                                break;
                            }
                        }
                    }
                    //if the current function is the target function.
                    if (!currFunction.isDeclaration() && currFunction.hasName() &&
                        currFunction.getName().str() == checkFunctionName) {
                        currFunction.getAllMetadata(MDs);
                        TypePrintHelper::setFoldersPath(srcBaseDir, bitcodeOutDir);

                        for (auto &MD : MDs) {
                            if (MDNode *N = MD.second) {
                                if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                                    targetFName = subProgram->getFilename();
                                    tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir,
                                                                                preprocessedPrefix);
                                    if (TypePrintHelper::requiredPreprocessingFiles.find(tmpFilePath) ==
                                        TypePrintHelper::requiredPreprocessingFiles.end()) {
                                        TypePrintHelper::requiredPreprocessingFiles.insert(tmpFilePath);
                                    }
                                    tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir,
                                                                                includePrefix);
                                    if (TypePrintHelper::requiredIncludeFiles.find(tmpFilePath) ==
                                        TypePrintHelper::requiredIncludeFiles.end()) {
                                        TypePrintHelper::requiredIncludeFiles.insert(tmpFilePath);
                                    }
                                    break;
                                }
                            }
                        }


                        //InterProceduralRA<CropDFS> &range_analysis = getAnalysis<InterProceduralRA<CropDFS>>();
                        std::set<int> cArg;
                        std::set<int> uArg;
                        std::vector<Value *> callStack;
                        std::map<unsigned, Value *> callerArguments;
                        callerArguments.clear();
                        DefIoctlInstVisitor *currFuncVis = new DefIoctlInstVisitor(&currFunction, cArg, uArg,
                                                                                   callerArguments, callStack, nullptr, 0);
                        // start analyzing
                        currFuncVis->analyze();
                        for (auto a:TypePrintHelper::requiredIncludeFiles) {
                            dbgs() << "Includes file:" << a << "\n";
                        }
                        for (auto a:TypePrintHelper::requiredPreprocessingFiles) {
                            dbgs() << "Preprocessed file:" << a << "\n";
                        }

                        dbgs() << "ALL PREPROCESSED FILES:\n";
                        for(auto a:all_c_preprocessed_files) {
                            dbgs() << "Compl Preprocessed file:" << a << "\n";
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

    char IoctlCmdCheckerPass::ID = 0;
    static RegisterPass<IoctlCmdCheckerPass> x("v4l2-processor", "V4L2 Processor", false, true);
}