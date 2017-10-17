//
// Created by machiry on 5/10/17.
//

#include "DefIoctlInstVisitor.h"

#include <CFGUtils.h>
#include "TypePrintHelper.h"
#include "../../IoctlCmdFinder/include/TypePrintHelper.h"
#include <iostream>

using namespace llvm;
using namespace std;
#define MAX_FUNC_DEPTH 10
namespace IOCTL_CHECKER {
    // the main analysis function.
    void DefIoctlInstVisitor::analyze() {
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

        bool is_handled;
        while(!processQueue.empty()) {
            BasicBlock *currBB = processQueue[0];
            // remove first element
            processQueue.erase(processQueue.begin());
            if(this->visitBB(currBB)) {
                dbgs() << "Found a common structure\n";
            }
            TerminatorInst *terminInst = currBB->getTerminator();
            is_handled = false;

            if(terminInst != nullptr) {
                // first check if the instruction is affected by cmd value
                if(terminInst->getNumSuccessors() > 1 && isCmdAffected(terminInst)) {
                    // is this a switch?
                    SwitchInst *dstSwitch = dyn_cast<SwitchInst>(currBB->getTerminator());
                    if(dstSwitch != nullptr) {
                        //dbgs() << "Trying switch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                        // is switch handle cmd switch.
                        is_handled = handleCmdSwitch(dstSwitch);
                    }
                }
                if(is_handled) {
                    std::vector<BasicBlock*> reachableBBs;
                    reachableBBs.clear();
                    BBTraversalHelper::getAllReachableBBs(currBB, processQueue, reachableBBs);
                    //dbgs() << "Removing all successor BBs from:" << currBB->getName() << ":" << this->targetFunction->getName() << "\n";
                    for(unsigned int i=0; i < reachableBBs.size(); i++) {
                        // remove all reachable BBs as these are already handled.
                        processQueue.erase(std::remove(processQueue.begin(), processQueue.end(), reachableBBs[i]),
                                           processQueue.end());
                    }
                }
            } else {
                assert(false);
            }
        }
    }

    void DefIoctlInstVisitor::visitAllBBs(BasicBlock *startBB, std::set<BasicBlock*> &visitedBBs) {

        if(visitedBBs.find(startBB) == visitedBBs.end()) {
            visitedBBs.insert(startBB);
            // if no copy_from/to_user function is found?
            if(!this->visitBB(startBB)) {
                    // then visit its successors.
                for (auto sb = succ_begin(startBB), se = succ_end(startBB); sb != se; sb++) {
                        BasicBlock *currSucc = *sb;
                        this->visitAllBBs(currSucc, visitedBBs);
                }
            }
            visitedBBs.erase(startBB);
        }
    }

    bool DefIoctlInstVisitor::handleCmdSwitch(SwitchInst *targetSwitchInst) {
        Value *targetSwitchCond = targetSwitchInst->getCondition();
        std::set<BasicBlock*> visitedInThisIteration;
        if(this->isCmdAffected(targetSwitchCond)) {
            for(auto cis=targetSwitchInst->case_begin(), cie=targetSwitchInst->case_end(); cis != cie; cis++) {
                ConstantInt *cmdVal = cis.getCaseValue();
                BasicBlock *caseStartBB = cis.getCaseSuccessor();
                std::set<BasicBlock*> visitedBBs;
                visitedBBs.clear();
                // start the print
                dbgs() << "Found Cmd:" << cmdVal->getValue().getZExtValue() << ":START\n";
                // Now visit all the successors
                visitAllBBs(caseStartBB, visitedBBs);
                dbgs() << "Found Cmd:" << cmdVal->getValue().getZExtValue() << ":END\n";
            }
            return true;
        }
        return false;
    }

    // visitor instructions
    void DefIoctlInstVisitor::visitCallInst(CallInst &I) {
        Function *dstFunc = I.getCalledFunction();
        if(dstFunc != nullptr) {
            if(dstFunc->isDeclaration()) {
                if(dstFunc->hasName()) {
                    string calledfunc = I.getCalledFunction()->getName().str();
                    Value *targetOperand = nullptr;
                    Value *srcOperand = nullptr;
                    if(calledfunc.find("_copy_from_user") != string::npos) {
                        //dbgs() << "copy_from_user:\n";
                        if(I.getNumArgOperands() >= 1) {
                            targetOperand = I.getArgOperand(0);
                        }
                        if(I.getNumArgOperands() >= 2) {
                            srcOperand = I.getArgOperand(1);
                        }
                    }
                    if(calledfunc.find("_copy_to_user") != string::npos) {
                        // some idiot doesn't know how to parse
                        /*dbgs() << "copy_to_user:\n";
                        srcOperand = I.getArgOperand(0);
                        targetOperand = I.getArgOperand(1);*/
                    }
                    if(targetOperand != nullptr) {
                        TypePrintHelper::typeOutputHandler(targetOperand, dbgs(), this);
                    }
                }
            }
        } else {
            // check if maximum function depth is reached.
            if(this->curr_func_depth > MAX_FUNC_DEPTH) {
                return;
            }
            if(!I.isInlineAsm()) {
                // TODO: push the function pointer handling code

            }
        }


    }

    bool DefIoctlInstVisitor::visitBB(BasicBlock *BB) {
        //dbgs() << "START TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        _super->visit(BB->begin(), BB->end());
        //dbgs() << "END TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        return false;
    }
}