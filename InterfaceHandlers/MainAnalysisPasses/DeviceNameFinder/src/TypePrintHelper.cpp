//
// Created by machiry on 4/26/17.
//

#include <set>
#include <FileUtils.h>
#include "TypePrintHelper.h"
//#include "IOInstVisitor.h"
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {

    std::set<std::string> TypePrintHelper::requiredIncludeFiles;
    std::set<std::string> TypePrintHelper::requiredPreprocessingFiles;
    string TypePrintHelper::srcKernelDir;
    string TypePrintHelper::llvmBcFile;

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
}