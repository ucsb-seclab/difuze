//
// Created by machiry on 4/24/17.
//

#include "IOInstVisitor.h"
#include <CFGUtils.h>
#include "TypePrintHelper.h"
#include <iostream>
#include <llvm/IR/Operator.h>

using namespace llvm;
using namespace std;
namespace IOCTL_CHECKER {
    // the main analysis function.
    void IOInstVisitor::analyze() {
        // No what we need is:
        // Traverse the CFG in Breadth first order.
        std::vector<BasicBlock*> processQueue;

        std::vector<std::vector<BasicBlock *> *> *traversalOrder =
                BBTraversalHelper::getSCCTraversalOrder(*(this->targetFunction));
        for(unsigned int i = 0; i < traversalOrder->size(); i++) {
            // current strongly connected component.
            std::vector<BasicBlock *> *currSCC = (*(traversalOrder))[i];
            for (unsigned int j = 0; j < currSCC->size(); j++) {
                BasicBlock *currBB = (*currSCC)[j];
                processQueue.insert(processQueue.end(), currBB);
            }

        }
        std::set<BasicBlock*> totalVisitedBBs;
        while(!processQueue.empty()) {
            BasicBlock *currBB = processQueue[0];
            // remove first element
            processQueue.erase(processQueue.begin());
            this->visitBB(currBB);
        }
    }

    void IOInstVisitor::visitICmpInst(ICmpInst &I) {
        // check if we doing cmd == comparision.
        if(I.isEquality()) {
            Value *op1 = I.getOperand(0);
            Value *op2 = I.getOperand(1);
            Value *targetValueArg = nullptr;
            Value *nonConst = nullptr;
            //Range targetRange = this->range_analysis->getRange(op1);
            ConstantInt *cInt = dyn_cast<ConstantInt>(op1);
            if(cInt) {
                targetValueArg = op1;
                nonConst = op2;
            } else {
                cInt = dyn_cast<ConstantInt>(op2);
                if(cInt) {
                    targetValueArg = op2;
                    nonConst = op1;
                }
            }
            // OK, we have a constant comparision.
            if(targetValueArg != nullptr) {
                // back trace
                std::string resultSpacing;
                if(this->backTrackUserType(nonConst, resultSpacing)) {
                    dbgs() << resultSpacing << "POSSIBLE VALUES (1) :\n";
                    dbgs() << resultSpacing << "BR VALUE:" << cInt->getZExtValue() << ":" << cInt->getZExtValue() << "\n";
                }
            }

        }
    }


    bool IOInstVisitor::backTrackUserType(Value *currVal, std::string &spacing) {
        Value *strippedVal = currVal->stripPointerCasts();
        std::string currSpacing;
        // check if this is the user arg.
        if(currVal == this->userArg) {
            //dbgs() << "Equals:" << *currVal << ",Stipped:" << *strippedVal << "\n";
            TypePrintHelper::printType(this->targetUserType, dbgs());
            spacing.append("  ");
            return true;
        }

        Instruction *targetInst = dyn_cast<Instruction>(currVal);
        if(targetInst == nullptr) {
            targetInst = dyn_cast<Instruction>(strippedVal);
        }

        if(targetInst != nullptr) {
            LoadInst *isLoad = dyn_cast<LoadInst>(targetInst);
            if(isLoad != nullptr) {
                // Handle Load instruction.
                Value *targetOperand = isLoad->getPointerOperand();

                bool to_ret = this->backTrackUserType(targetOperand, spacing);
                return to_ret;
            } else {
                GetElementPtrInst *currGep = dyn_cast<GetElementPtrInst>(targetInst);
                if(currGep != nullptr) {
                    Value* srcPointer = currGep->getPointerOperand();
                    /*GEPOperator *gep = dyn_cast<GEPOperator>(currGep->getPointerOperand());
                    if(gep && gep->getNumOperands() > 0 && gep->getPointerOperand()) {
                        srcPointer = gep->getPointerOperand();
                    }*/
                    if(this->backTrackUserType(srcPointer, currSpacing)) {
                        //dbgs() << *currGep << ", Num Oper:" << currGep->getNumOperands() << "\n";
                        //dbgs() << "Source:" << *srcPointer << "\n";
                        if (currGep->getNumOperands() > 2) {
                            if (ConstantInt *CI = dyn_cast<ConstantInt>(currGep->getOperand(2))) {
                                unsigned long structFieldId = CI->getZExtValue();
                                dbgs() << currSpacing << "field:" << structFieldId << "\n";
                                spacing.append(currSpacing);
                                spacing.append("  ");
                                return true;
                            } else {
                                dbgs() << "Indexing a structure with a variable field id, WTF??\n";
                                assert(false);
                            }
                        } else {
                            // array..
                            if (ConstantInt *CI = dyn_cast<ConstantInt>(currGep->getOperand(1))) {
                                unsigned long arrayIndex = CI->getZExtValue();
                                dbgs() << currSpacing << "arr:" << arrayIndex << "\n";
                                spacing.append(currSpacing);
                                spacing.append("  ");
                                return true;
                            } else {
                                // OK, we are indexing an array with variable index. we do not care.

                            }
                        }
                    }
                }
            }
        }
        return false;
    }


    void IOInstVisitor::visitSwitchInst(SwitchInst &I) {
        Value *targetSwitchCond = I.getCondition();
        std::string resultSpacing;
        if(this->backTrackUserType(targetSwitchCond, resultSpacing)) {
            dbgs() << resultSpacing << "POSSIBLE VALUES (" << I.getNumCases() << ")\n";
            for(auto cis=I.case_begin(), cie=I.case_end(); cis != cie; cis++) {
                ConstantInt *cmdVal = (*cis).getCaseValue();
                // start the print
                dbgs() << resultSpacing << "Value:" << cmdVal->getValue().getZExtValue() << "\n";
            }
        }
    }



    bool IOInstVisitor::visitBB(BasicBlock *BB) {
        //dbgs() << "START TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        _super->visit(BB->begin(), BB->end());
        //dbgs() << "END TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        return false;
    }


}
