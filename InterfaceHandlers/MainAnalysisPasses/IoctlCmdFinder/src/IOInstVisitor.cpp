//
// Created by machiry on 4/24/17.
//

#include "IOInstVisitor.h"
#include <CFGUtils.h>
#include "TypePrintHelper.h"
#include <iostream>

using namespace llvm;
using namespace std;
#define MAX_FUNC_DEPTH 10
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

        bool is_handled;
        std::set<BasicBlock*> totalVisitedBBs;
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
                        is_handled = handleCmdSwitch(dstSwitch, totalVisitedBBs);
                    } else {
                        //dbgs() << "START:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                        // not switch?, check if the branch instruction
                        // if this is branch, handle cmd branch
                        BranchInst *brInst = dyn_cast<BranchInst>(terminInst);
                        if(brInst == nullptr) {
                            dbgs() << "Culprit:" << "\n";
                            currBB->dump();
                        }
                        assert(brInst != nullptr);
                        is_handled = handleCmdCond(brInst, totalVisitedBBs);
                        //dbgs() << "END:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
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

    void IOInstVisitor::visitAllBBs(BasicBlock *startBB, std::set<BasicBlock*> &visitedBBs,
                                    std::set<BasicBlock*> &totalVisited, std::set<BasicBlock*> &visitedInThisIteration) {
        TerminatorInst *terminInst;
        bool is_handled;

        if(visitedBBs.find(startBB) == visitedBBs.end() && totalVisited.find(startBB) == totalVisited.end()) {
            visitedBBs.insert(startBB);
            visitedInThisIteration.insert(startBB);
            // if no copy_from/to_user function is found?
            if(!this->visitBB(startBB)) {

                terminInst = startBB->getTerminator();
                is_handled = false;

                if(terminInst != nullptr) {
                    // first check if the instruction is affected by cmd value
                    if (terminInst->getNumSuccessors() > 1 && isCmdAffected(terminInst)) {
                        // is this a switch?
                        SwitchInst *dstSwitch = dyn_cast<SwitchInst>(startBB->getTerminator());
                        if (dstSwitch != nullptr) {
                            //dbgs() << "Trying switch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                            // is switch handle cmd switch.
                            is_handled = handleCmdSwitch(dstSwitch, totalVisited);
                        } else {
                            //dbgs() << "START:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                            // not switch?, check if the branch instruction
                            // if this is branch, handle cmd branch
                            BranchInst *brInst = dyn_cast<BranchInst>(terminInst);
                            if (brInst == nullptr) {
                                dbgs() << "Culprit:" << "\n";
                                startBB->dump();
                            }
                            assert(brInst != nullptr);
                            is_handled = handleCmdCond(brInst, totalVisited);
                            //dbgs() << "END:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                        }
                    }
                }
                if(!is_handled) {
                    // then visit its successors.
                    for (auto sb = succ_begin(startBB), se = succ_end(startBB); sb != se; sb++) {
                        BasicBlock *currSucc = *sb;
                        this->visitAllBBs(currSucc, visitedBBs, totalVisited, visitedInThisIteration);
                    }
                }
            }
            visitedBBs.erase(startBB);
        }
    }

    bool IOInstVisitor::handleCmdSwitch(SwitchInst *targetSwitchInst, std::set<BasicBlock*> &totalVisited) {
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
                visitAllBBs(caseStartBB, visitedBBs, totalVisited, visitedInThisIteration);
                dbgs() << "Found Cmd:" << cmdVal->getValue().getZExtValue() << ":END\n";
            }
            totalVisited.insert(visitedInThisIteration.begin(), visitedInThisIteration.end());
            return true;
        }
        return false;
    }

    bool IOInstVisitor::handleCmdCond(BranchInst *I, std::set<BasicBlock*> &totalVisited) {
        // OK, So, this is a branch instruction,
        // We could possibly have cmd value compared against.
        if(this->currCmdValue != nullptr) {
            //dbgs() << "CMDCOND:::" << *(this->currCmdValue) << "\n";
            std::set<BasicBlock*> visitedBBs;
            visitedBBs.clear();
            std::set<BasicBlock*> visitedInThisIteration;
            Value *cmdValue = this->currCmdValue;
            ConstantInt *cInt = dyn_cast<ConstantInt>(cmdValue);
            if(cInt != nullptr) {
                dbgs() << "Found Cmd(BR):" << cInt->getZExtValue() << "," << cInt->getZExtValue() << ":START\n";
                this->currCmdValue = nullptr;
                for (auto sb = succ_begin(I->getParent()), se = succ_end(I->getParent()); sb != se; sb++) {
                    BasicBlock *currSucc = *sb;
                    if(totalVisited.find(currSucc) == totalVisited.end()) {
                        this->visitAllBBs(currSucc, visitedBBs, totalVisited, visitedInThisIteration);
                    }
                }
                dbgs() << "Found Cmd(BR):" << cInt->getZExtValue() << "," << cInt->getZExtValue() << ":END\n";
                // insert all the visited BBS into total visited.
                totalVisited.insert(visitedInThisIteration.begin(), visitedInThisIteration.end());
                return true;
            }
            /*Range targetRange = this->range_analysis->getRange(cmdValue);
            if(!targetRange.isMaxRange() && (targetRange.getLower() != 0 || targetRange.getUpper() != 0)) {
                //dbgs() << "Target BR Instruction:" << *(this->currCmdValue) << "\n";
                dbgs() << "Found Cmd(BR):" << targetRange.getLower() << "," << targetRange.getUpper() << ":START\n";
                this->currCmdValue = nullptr;
                for (auto sb = succ_begin(I->getParent()), se = succ_end(I->getParent()); sb != se; sb++) {
                    BasicBlock *currSucc = *sb;
                    if(totalVisited.find(currSucc) == totalVisited.end()) {
                        this->visitAllBBs(currSucc, visitedBBs, totalVisited, visitedInThisIteration);
                    }
                }
                dbgs() << "Found Cmd(BR):" << targetRange.getLower() << "," << targetRange.getUpper() << ":END\n";
                // insert all the visited BBS into total visited.
                totalVisited.insert(visitedInThisIteration.begin(), visitedInThisIteration.end());
                return true;
            }*/
        }
        return false;
    }

    Function* IOInstVisitor::getFinalFuncTarget(CallInst *I) {
        if(I->getCalledFunction() == nullptr) {
            Value *calledVal = I->getCalledValue();
            if(dyn_cast<Function>(calledVal)) {
                return dyn_cast<Function>(calledVal->stripPointerCasts());
            }
        }
        return I->getCalledFunction();

    }

    // visitor instructions
    void IOInstVisitor::visitCallInst(CallInst &I) {
        Function *dstFunc = this->getFinalFuncTarget(&I);
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
                    if(srcOperand != nullptr) {
                        // sanity, this should be user value argument.
                        // only consider value arguments.
                        if(!this->isArgAffected(srcOperand)) {
                            // dbgs() << "Found a copy from user from non-user argument\n";
                            //srcOperand = nullptr;
                            //targetOperand = nullptr;
                        }
                    }
                    if(targetOperand != nullptr) {
                        TypePrintHelper::typeOutputHandler(targetOperand, dbgs(), this);
                    }
                }
            } else {
                // check if maximum function depth is reached.
                if(this->curr_func_depth > MAX_FUNC_DEPTH) {
                    return;
                }
                // we need to follow the called function, only if this is not recursive.
                if(std::find(this->callStack.begin(), this->callStack.end(), &I) == this->callStack.end()) {
                    std::vector<Value*> newCallStack;
                    newCallStack.insert(newCallStack.end(), this->callStack.begin(), this->callStack.end());
                    newCallStack.insert(newCallStack.end(), &I);
                    std::set<int> cmdArg;
                    std::set<int> uArg;
                    std::map<unsigned, Value*> callerArgMap;
                    cmdArg.clear();
                    uArg.clear();
                    callerArgMap.clear();
                    // get propagation info
                    this->getArgPropogationInfo(&I, cmdArg, uArg, callerArgMap);
                    // analyze only if one of the argument is a command or argument
                    if(cmdArg.size() > 0 || uArg.size() > 0) {
                        IOInstVisitor *childFuncVisitor = new IOInstVisitor(dstFunc, cmdArg, uArg, callerArgMap,
                                                                            newCallStack, this,
                                                                            this->curr_func_depth + 1);
                        childFuncVisitor->analyze();
                    }

                }
            }
        } else {
            // check if maximum function depth is reached.
            if(this->curr_func_depth > MAX_FUNC_DEPTH) {
                return;
            }
            if(!I.isInlineAsm()) {
                // TODO: Push the function pointer handling code.

            }
        }


    }

    void IOInstVisitor::visitICmpInst(ICmpInst &I) {
        // check if we doing cmd == comparision.
        if(I.isEquality()) {
            Value *op1 = I.getOperand(0);
            Value *op2 = I.getOperand(1);
            Value *targetValueArg = nullptr;
            if(this->isCmdAffected(op1)) {
                targetValueArg = op2;
            } else if(this->isCmdAffected(op2)) {
                targetValueArg = op1;
            }
            if(targetValueArg != nullptr) {
                //dbgs() << "Setting value for:" << I << "\n";
                this->currCmdValue = targetValueArg;
            }
        }
    }

    bool IOInstVisitor::visitBB(BasicBlock *BB) {
        //dbgs() << "START TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        _super->visit(BB->begin(), BB->end());
        //dbgs() << "END TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        return false;
    }

    void IOInstVisitor::getArgPropogationInfo(CallInst *I, std::set<int> &cmdArg, std::set<int> &uArg,
                                              std::map<unsigned, Value*> &callerArgInfo) {
        int curr_arg_indx = 0;
        for(User::op_iterator arg_begin = I->arg_begin(), arg_end = I->arg_end();
            arg_begin != arg_end; arg_begin++) {
            Value *currArgVal = (*arg_begin).get();
            if(this->isCmdAffected(currArgVal)) {
                cmdArg.insert(curr_arg_indx);
            }
            if(this->isArgAffected(currArgVal)) {
                uArg.insert(curr_arg_indx);
            }
            callerArgInfo[curr_arg_indx] = currArgVal;
            curr_arg_indx++;
        }
    }
}
