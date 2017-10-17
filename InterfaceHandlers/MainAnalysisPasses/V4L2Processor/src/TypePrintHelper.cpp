//
// Created by machiry on 4/26/17.
//

#include <set>
#include <FileUtils.h>
#include "TypePrintHelper.h"
#include "IOInstVisitor.h"
#include "DefIoctlInstVisitor.h"
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {

    std::set<std::string> TypePrintHelper::requiredIncludeFiles;
    std::set<std::string> TypePrintHelper::requiredPreprocessingFiles;
    string TypePrintHelper::srcKernelDir;
    string TypePrintHelper::llvmBcFile;

    Type* getTargetStType(Instruction *targetbc) {
        BitCastInst *tarcs = dyn_cast<BitCastInst>(targetbc);
        if(tarcs != nullptr) {
            return tarcs->getSrcTy();
        }
        GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(targetbc);
        if(gepInst != nullptr) {
            return gepInst->getSourceElementType();
        }
        return nullptr;
    }

    Type* getTargetTypeForValue(Value *targetVal, DefIoctlInstVisitor *currFunction, llvm::raw_ostream &to_out, bool &is_printed) {
        targetVal = targetVal->stripPointerCasts();
        std::set<Value*> all_uses;
        all_uses.clear();
        for(auto us=targetVal->use_begin(), ue=targetVal->use_end(); us != ue; us++) {
            all_uses.insert((*us).get());
        }
        if(all_uses.size() > 1) {
            to_out << "More than one use :(\n";
        }
        for(auto sd:all_uses) {
            GlobalVariable *gv = dyn_cast<GlobalVariable>(sd);
            if(gv) {
                return gv->getType();
            }
        }

        // OK, unable to get the type.
        // Now follow the arguments.
        if(currFunction) {
            if(currFunction->callerArgs.find(targetVal) != currFunction->callerArgs.end()) {
                // OK, this is function arguments and we have propagation information.
                Value *callerArg = currFunction->callerArgs[targetVal];
                Type *retVal = TypePrintHelper::typeOutputHandler(callerArg, to_out, currFunction->calledVisitor);
                is_printed = true;
                return retVal;
            }
        }
        return nullptr;
    }

    void printTypeRecursive(Type *targetType, llvm::raw_ostream &to_out,
                                             std::string &prefix_space) {
        if(targetType->isStructTy() || targetType->isPointerTy()) {
            if(targetType->isPointerTy()) {
                targetType = targetType->getContainedType(0);
            }
            bool is_handled = false;
            if(targetType->isStructTy()) {
                string src_st_name = targetType->getStructName().str();
                if(src_st_name.find(".anon") != string::npos) {
                    is_handled = true;
                    // OK, this is anonymous struct or union.
                    to_out << prefix_space << src_st_name << ":STARTELEMENTS:\n";
                    string child_space = prefix_space;
                    child_space = child_space + "  ";
                    for(unsigned int curr_no=0; curr_no<targetType->getStructNumElements(); curr_no++) {
                        // print by adding space
                        printTypeRecursive(targetType->getStructElementType(0), to_out, child_space);
                    }
                    to_out << prefix_space << src_st_name << ":ENDELEMENTS:\n";
                }
            }
            if(!is_handled) {
                // Regular structure, print normally.
                to_out << prefix_space << *targetType << "\n";
            }
        } else {
            to_out << prefix_space << *targetType << "\n";
        }

    }

    void TypePrintHelper::setFoldersPath(string &srcKDir, string &lbcfile) {
        TypePrintHelper::srcKernelDir = srcKDir;
        TypePrintHelper::llvmBcFile = lbcfile;
    }

    void TypePrintHelper::printType(Type *targetType, llvm::raw_ostream &to_out) {
        to_out << "STARTTYPE\n";
        string curr_prefix = "";
        printTypeRecursive(targetType, to_out, curr_prefix);
        to_out << "ENDTYPE\n";
    }

    Type* TypePrintHelper::typeOutputHandler(Value *targetVal, llvm::raw_ostream &to_out, DefIoctlInstVisitor *currFunc) {
        Type *retType = nullptr;
        bool is_printed = false;
        if (!dyn_cast<Instruction>(targetVal)) {

            Type *targetType = getTargetTypeForValue(targetVal, currFunc, to_out, is_printed);
            if (targetType != nullptr) {
                if(!is_printed) {
                    //TypePrintHelper::addRequiredFile(dyn_cast<Instruction>(targetVal));
                    printType(targetType, to_out);
                }
            } else {
                to_out << " no TYPE for:" << *targetVal << "\n";
            }
            retType = targetType;
        } else {
            Type *targetType = getTargetStType(dyn_cast<Instruction>(targetVal));
            if(targetType == nullptr) {
                targetType = getTargetTypeForValue(targetVal, currFunc, to_out, is_printed);
            }
            if (targetType != nullptr) {
                if(!is_printed) {
                    //TypePrintHelper::addRequiredFile(dyn_cast<Instruction>(targetVal));
                    printType(targetType, to_out);
                }
            } else {

                to_out << " no TYPE for:" << *targetVal << "\n";
            }
            retType = targetType;
        }

        return retType;
    }
}