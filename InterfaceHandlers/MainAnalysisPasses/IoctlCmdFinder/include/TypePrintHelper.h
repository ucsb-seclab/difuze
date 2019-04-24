//
// Created by machiry on 4/26/17.
//

#ifndef PROJECT_TYPEPRINTHELPER_H
#define PROJECT_TYPEPRINTHELPER_H

#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"
//#include "dsa/DSGraph.h"
//#include "dsa/DataStructure.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/CFG.h"

using namespace llvm;
using namespace std;
namespace IOCTL_CHECKER {
    class IOInstVisitor;
    class TypePrintHelper {
    public:
        static std::set<std::string> requiredIncludeFiles;
        static std::set<std::string> requiredPreprocessingFiles;
        static std::string srcKernelDir;
        static std::string llvmBcFile;
        static void setFoldersPath(string &srcKDir, string &lbcfile);
        static Type* typeOutputHandler(Value *targetVal, llvm::raw_ostream &to_out, IOInstVisitor *currFunction);
        static void addRequiredFile(Instruction *currInstruction);

        static Type* getInstructionTypeRecursive(Value *currInstr, std::set<Instruction*> &visited);
    };
}

#endif //PROJECT_TYPEPRINTHELPER_H
