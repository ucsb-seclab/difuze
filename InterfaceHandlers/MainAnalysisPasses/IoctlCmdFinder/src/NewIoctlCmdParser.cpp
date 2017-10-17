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
#include "llvm/Support/CommandLine.h"
#include "TypePrintHelper.h"


using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {


    static cl::opt<std::string> checkFunctionName("ioctlFunction",
                                                  cl::desc("Function which is to be considered as entry point "
                                                                   "into the driver"),
                                                  cl::value_desc("full name of the function"), cl::init(""));

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

        bool runOnModule(Module &m) override {
            dbgs() << "Provided Function Name:" << checkFunctionName << "\n";
            std::set<string> all_c_preprocessed_files;
            //unimelb::WrappedRangePass &range_analysis = getAnalysis<unimelb::WrappedRangePass>();
            for(Module::iterator mi = m.begin(), ei = m.end(); mi != ei; mi++) {
                std::string tmpFilePath;
                std::string includePrefix = ".includes";
                std::string preprocessedPrefix = ".preprocessed";

                Function &currFunction = *mi;
                SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
                std::string targetFName;
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
                if(!currFunction.isDeclaration() && currFunction.hasName() &&
                   currFunction.getName().str() == checkFunctionName) {
                    TypePrintHelper::setFoldersPath(srcBaseDir, bitcodeOutDir);
                    TypePrintHelper::addRequiredFile(currFunction.getEntryBlock().getFirstNonPHIOrDbg());

                    for (auto &MD : MDs) {
                        if (MDNode *N = MD.second) {
                            if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                                targetFName = subProgram->getFilename();
                                tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir, preprocessedPrefix);
                                if(TypePrintHelper::requiredPreprocessingFiles.find(tmpFilePath) == TypePrintHelper::requiredPreprocessingFiles.end()) {
                                    TypePrintHelper::requiredPreprocessingFiles.insert(tmpFilePath);
                                }
                                tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir, includePrefix);
                                if(TypePrintHelper::requiredIncludeFiles.find(tmpFilePath) == TypePrintHelper::requiredIncludeFiles.end()) {
                                    TypePrintHelper::requiredIncludeFiles.insert(tmpFilePath);
                                }
                                break;
                            }
                        }
                    }

                    /*bool foundFile = false;
                    // get the file name of the ioctl function.
                    std::string targetFName;
                    SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
                    currFunction.getAllMetadata(MDs);
                    for (auto &MD : MDs) {
                        if (MDNode *N = MD.second) {
                            if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                                targetFName = subProgram->getFilename();
                                foundFile = true;
                            }
                        }
                    }
                    // print the path to includes and preprocessed files.
                    if(foundFile) {
                        std::string tmpFilePath;
                        std::string includePrefix = ".includes";
                        std::string preprocessedPrefix = ".preprocessed";
                        tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir, includePrefix);
                        dbgs() << "Includes file:" << tmpFilePath << "\n";
                        tmpFilePath = FileUtils::getNewRelativePath(srcBaseDir, targetFName, bitcodeOutDir, preprocessedPrefix);
                        dbgs() << "Preprocessed file:" << tmpFilePath << "\n";

                    } else {
                        dbgs() << "Include file:No Debug information in the bitcode file.\n";
                        dbgs() << "Preprocessed file: No Debug information in the bitcode file.\n";
                    }*/

                    //InterProceduralRA<CropDFS> &range_analysis = getAnalysis<InterProceduralRA<CropDFS>>();
                    std::set<int> cArg;
                    std::set<int> uArg;
                    std::vector<Value*> callStack;
                    std::map<unsigned, Value*> callerArguments;
                    cArg.insert(1);
                    uArg.insert(2);
                    callerArguments.clear();
                    IOInstVisitor *currFuncVis = new IOInstVisitor(&currFunction, cArg, uArg, callerArguments,
                                                                   callStack, nullptr, 0);
                    // start analyzing
                    currFuncVis->analyze();
                    for(auto a:TypePrintHelper::requiredIncludeFiles) {
                        dbgs() << "Includes file:" << a << "\n";
                    }
                    for(auto a:TypePrintHelper::requiredPreprocessingFiles) {
                        dbgs() << "Preprocessed file:" << a << "\n";
                    }
                    dbgs() << "ALL PREPROCESSED FILES:\n";
                    for(auto a:all_c_preprocessed_files) {
                        dbgs() << "Compl Preprocessed file:" << a << "\n";
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
    static RegisterPass<IoctlCmdCheckerPass> x("new-ioctl-cmd-parser", "IOCTL Command Parser", false, true);
}