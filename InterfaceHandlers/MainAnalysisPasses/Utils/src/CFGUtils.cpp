//
// Created by machiry on 12/3/16.
//

#include "CFGUtils.h"

namespace IOCTL_CHECKER {

    std::vector<std::vector<BasicBlock *> *>* BBTraversalHelper::getSCCTraversalOrder(Function &currF) {
        std::vector<std::vector<BasicBlock *> *> *bbTraversalList = new std::vector<std::vector<BasicBlock *> *>();
        const Function *F = &currF;
        for (scc_iterator<const Function *> I = scc_begin(F), IE = scc_end(F); I != IE; ++I) {
            // Obtain the vector of BBs in this SCC and print it out.
            const std::vector<const BasicBlock *> &SCCBBs = *I;
            std::vector<const BasicBlock *> *currSCC = new std::vector<const BasicBlock *>();
            for (std::vector<const BasicBlock *>::const_iterator BBI = SCCBBs.begin(),
                         BBIE = SCCBBs.end();
                 BBI != BBIE; ++BBI) {
                currSCC->insert(currSCC->begin(), (*BBI));
            }
            std::vector<BasicBlock *> *newCurrSCC = new std::vector<BasicBlock *>();
            for (unsigned int i = 0; i < currSCC->size(); i++) {
                for (Function::iterator b = currF.begin(), be = currF.end(); b != be; ++b) {
                    BasicBlock *BB = &(*b);
                    if (BB == (*currSCC)[i]) {
                        newCurrSCC->insert(newCurrSCC->begin(), BB);
                        break;
                    }
                }
            }
            delete (currSCC);
            bbTraversalList->insert(bbTraversalList->begin(), newCurrSCC);
        }
        return bbTraversalList;
    }

    unsigned long BBTraversalHelper::getNumTimesToAnalyze(std::vector<BasicBlock *> *currSCC) {
        /***
             * get number of times all the loop basicblocks should be analyzed.
             *  This is same as the longest use-def chain in the provided
             *  strongly connected component.
             *
             *  Why is this needed?
             *  This is needed because we want to ensure that all the
             *  information inside the loops have been processed.
             */

        std::map<BasicBlock *, unsigned long> useDefLength;
        useDefLength.clear();

        std::set<BasicBlock *> useDefChain;
        useDefChain.clear();
        std::set<BasicBlock *> visitedBBs;
        std::set<BasicBlock *> currBBToAnalyze;
        currBBToAnalyze.clear();
        unsigned long numToAnalyze = 1;

        for(BasicBlock *currBBMain:(*currSCC)) {
            if(useDefLength.find(currBBMain) == useDefLength.end()) {
                currBBToAnalyze.clear();
                currBBToAnalyze.insert(currBBToAnalyze.end(), currBBMain);
                useDefChain.clear();
                useDefChain.insert(useDefChain.end(), currBBMain);
                visitedBBs.clear();
                while (!currBBToAnalyze.empty()) {
                    for (auto currBB:currBBToAnalyze) {
                        visitedBBs.insert(visitedBBs.end(), currBB);
                        for (BasicBlock::iterator bstart = currBB->begin(), bend = currBB->end();
                             bstart != bend; bstart++) {
                            Instruction *currIns = &(*bstart);

                            for(int jkk=0;jkk < currIns->getNumOperands();jkk++) {
                                Value *i=currIns->getOperand(jkk);
                                if (Instruction *Inst = dyn_cast<Instruction>(i)) {
                                    BasicBlock *useBB = Inst->getParent();
                                    if (std::find(currSCC->begin(), currSCC->end(), useBB) != currSCC->end()) {
                                        if (useDefChain.find(useBB) == useDefChain.end()) {
                                            useDefChain.insert(useDefChain.end(), useBB);
                                        }
                                    }
                                }
                            }
                        }
                    }


                    currBBToAnalyze.clear();

                    for(auto b:useDefChain) {
                        if(visitedBBs.find(b) == visitedBBs.end()) {
                            currBBToAnalyze.insert(currBBToAnalyze.end(), b);
                        }
                    }

                }

                for (BasicBlock *vicBB:useDefChain) {
                    if(useDefLength.find(vicBB) == useDefLength.end()) {
                        useDefLength[vicBB] = useDefChain.size();
                    }
                }
            }
        }

        for(auto b:(*currSCC)) {
            if(useDefLength.find(b) != useDefLength.end()) {
                if(numToAnalyze < useDefLength[b]) {
                    numToAnalyze = useDefLength[b];
                }
            }
        }

        return numToAnalyze;

    }


    // following functions are shamelessly copied from LLVM.
    bool isPotentiallyReachableFromMany(SmallVectorImpl<BasicBlock *> &Worklist, BasicBlock *StopBB) {

        // Limit the number of blocks we visit. The goal is to avoid run-away compile
        // times on large CFGs without hampering sensible code. Arbitrarily chosen.
        unsigned Limit = 32;
        SmallSet<const BasicBlock*, 64> Visited;
        do {
            BasicBlock *BB = Worklist.pop_back_val();
            if (!Visited.insert(BB).second)
                continue;
            if (BB == StopBB)
                return true;

            if (!--Limit) {
                // We haven't been able to prove it one way or the other. Conservatively
                // answer true -- that there is potentially a path.
                return true;
            }
            Worklist.append(succ_begin(BB), succ_end(BB));
        } while (!Worklist.empty());

        // We have exhausted all possible paths and are certain that 'To' can not be
        // reached from 'From'.
        return false;
    }

    bool BBTraversalHelper::isPotentiallyReachable(const Instruction *A, const Instruction *B) {
        assert(A->getParent()->getParent() == B->getParent()->getParent() &&
               "This analysis is function-local!");

        SmallVector<BasicBlock*, 32> Worklist;

        if (A->getParent() == B->getParent()) {
            // The same block case is special because it's the only time we're looking
            // within a single block to see which instruction comes first. Once we
            // start looking at multiple blocks, the first instruction of the block is
            // reachable, so we only need to determine reachability between whole
            // blocks.
            BasicBlock *BB = const_cast<BasicBlock *>(A->getParent());

            // Linear scan, start at 'A', see whether we hit 'B' or the end first.
            for (BasicBlock::const_iterator I = A->getIterator(), E = BB->end(); I != E;
                 ++I) {
                if (&*I == B)
                    return true;
            }

            // Can't be in a loop if it's the entry block -- the entry block may not
            // have predecessors.
            if (BB == &BB->getParent()->getEntryBlock())
                return false;

            // Otherwise, continue doing the normal per-BB CFG walk.
            Worklist.append(succ_begin(BB), succ_end(BB));

            if (Worklist.empty()) {
                // We've proven that there's no path!
                return false;
            }
        } else {
            Worklist.push_back(const_cast<BasicBlock*>(A->getParent()));
        }

        if (A->getParent() == &A->getParent()->getParent()->getEntryBlock())
            return true;
        if (B->getParent() == &A->getParent()->getParent()->getEntryBlock())
            return false;

        return isPotentiallyReachableFromMany(
                Worklist, const_cast<BasicBlock *>(B->getParent()));
    }

    // llvm copying ends

    bool BBTraversalHelper::isReachable(Instruction *startInstr, Instruction *endInstr,
                                        std::vector<Instruction*> *callSites) {
        /***
         * Check if endInst is reachable from startInstr following the call sites
         * in the provided vector.
         */

        // both belong to the same function.
        if(startInstr->getParent()->getParent() == endInstr->getParent()->getParent()) {
            return BBTraversalHelper::isPotentiallyReachable(startInstr, endInstr);
        }

        // OK, both instructions belongs to different functions.
        // we need to find if startInstr can reach the
        // call instruction that resulted in invocation of the function of
        // endInstr
        assert(callSites->size() > 0 && "How can this be possible? we have 2 instructions, that belong to "
                                                "different functions, yet the call sites stack is empty");

        Instruction *newEndInstr;
        for(long i=(callSites->size() - 1);i>=0; i--) {
            newEndInstr = (*callSites)[i];
            if(newEndInstr->getParent()->getParent() == startInstr->getParent()->getParent()) {
                if(BBTraversalHelper::isPotentiallyReachable(startInstr, newEndInstr)) {
                    return true;
                }
            }
        }
        return false;

    }

    void BBTraversalHelper::getAllReachableBBs(BasicBlock *startBB, std::vector<BasicBlock*> &targetBBs,
                                            std::vector<BasicBlock*> &reachableBBs) {
        for(unsigned int i=0; i < targetBBs.size(); i++) {
            BasicBlock *endBB = targetBBs[i];
            Instruction *dstInst = endBB->getFirstNonPHIOrDbg();
            if (BBTraversalHelper::isPotentiallyReachable(startBB->getFirstNonPHIOrDbg(), dstInst)) {
                reachableBBs.insert(reachableBBs.end(), endBB);
            }

        }
    }

}
