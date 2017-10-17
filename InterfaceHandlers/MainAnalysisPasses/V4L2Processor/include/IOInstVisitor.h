//
// Created by machiry on 4/24/17.
//

#ifndef PROJECT_IOINSTVISITOR_H
#define PROJECT_IOINSTVISITOR_H

#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"
//#include "dsa/DSGraph.h"
//#include "dsa/DataStructure.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/CFG.h"
//#include "RangeAnalysis.h"

using namespace llvm;
namespace IOCTL_CHECKER {
    class IOInstVisitor : public InstVisitor<IOInstVisitor> {
    public:

        Function *targetFunction;

        // Range analysis results.
        //RangeAnalysis *range_analysis;

        Value *userArg;
        Type *targetUserType;


        IOInstVisitor(Function *targetFunc, unsigned int user_arg_no) {
            // Initialize all values.
            _super = static_cast<InstVisitor *>(this);
            this->targetFunction = targetFunc;
            unsigned int farg_no=0;
            for(Function::arg_iterator farg_begin = targetFunc->arg_begin(), farg_end = targetFunc->arg_end();
                farg_begin != farg_end; farg_begin++) {
                Value *currfArgVal = &(*farg_begin);
                if(farg_no == user_arg_no) {
                    this->userArg = currfArgVal;
                    this->targetUserType = targetFunc->getFunctionType()->getParamType(farg_no);
                }
                farg_no++;
            }
            //this->range_analysis = curr_range_analysis;
        }

        virtual void visit(Instruction &I) {
#ifdef DEBUG_INSTR_VISIT
            dbgs() << "Visiting instruction:";
            I.print(dbgs());
            dbgs() << "\n";
#endif

            this->_super->visit(I);
        }

        // visitor functions.

        virtual void visitICmpInst(ICmpInst &I);

        virtual void visitSwitchInst(SwitchInst &I);

        bool visitBB(BasicBlock *BB);

        // main analysis function.
        void analyze();
    private:
        InstVisitor *_super;

        bool backTrackUserType(Value *currVal, std::string &spacing);
    };
}

#endif //PROJECT_IOINSTVISITOR_H
