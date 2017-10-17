//
// Created by machiry on 12/5/16.
//

#include "InstructionUtils.h"

using namespace llvm;

namespace IOCTL_CHECKER {


    bool InstructionUtils::isPointerInstruction(Instruction *I) {
        bool retVal = false;
        LoadInst *LI = dyn_cast<LoadInst>(I);
        if(LI) {
            retVal = true;
        }
        StoreInst *SI = dyn_cast<StoreInst>(I);
        if(SI) {
            retVal = true;
        }
        VAArgInst *VAAI = dyn_cast<VAArgInst>(I);
        if(VAAI) {
            retVal = true;
        }
        return retVal;
    }

    unsigned InstructionUtils::getLineNumber(Instruction &I)
    {

        const DebugLoc &currDC = I.getDebugLoc();
        if(currDC) {
            return currDC.getLine();
        }
        return -1;
    }

    std::string InstructionUtils::getInstructionName(Instruction *I) {
        if(I->hasName()) {
            return I->getName().str();
        } else {
            return "No Name";
        }
    }

    std::string InstructionUtils::getValueName(Value *v) {
        if(v->hasName()) {
            return v->getName().str();
        } else {
            return "No Name";
        }
    }
}
