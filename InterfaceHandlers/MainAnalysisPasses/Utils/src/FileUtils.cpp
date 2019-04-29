//
// Created by machiry on 5/1/17.
//
#include <iostream>
#include <llvm/Support/Debug.h>
#include "llvm/IR/DebugInfo.h"
#include "FileUtils.h"

using namespace llvm;

namespace IOCTL_CHECKER {
    std::string FileUtils::getNewRelativePath(std::string &srcBaseDir, std::string &srcFilePath, std::string &bitCodeDir,
                                       std::string &suffix) {
        std::string relativePath;
        bool is_handled = false;
        // if src file path starts with srcBaseDir
        if(!srcFilePath.compare(0, srcBaseDir.size(), srcBaseDir)) {
            relativePath = srcFilePath.substr(srcBaseDir.size());
            is_handled = true;
        } else {
            // if src file does not start, if may start with . or ..
            if(!srcFilePath.compare(0, 3, "../")) {
                relativePath = srcFilePath.substr(3);
                is_handled = true;
            } else {
                if(!srcFilePath.compare(0, 2, "./")) {
                    relativePath = srcFilePath.substr(2);
                    is_handled = true;
                } else {
                    // this is when the path is relative to the source directory
                    if(bitCodeDir.back() != '/') {
                        relativePath = "/";
                        relativePath.append(srcFilePath);                ;
                    } else {
                        relativePath = srcFilePath;
                    }
                    is_handled = true;
                }
            }
        }

        std::string to_ret = "";
        if(is_handled) {
            to_ret = bitCodeDir;
            // handle missing path separator
            if(*(bitCodeDir.end()) != '/' && *(relativePath.begin()) != '/') {
                to_ret.append("/");
            }
            to_ret.append(relativePath.substr(0, relativePath.length() - 2));
            to_ret.append(suffix);
        }
        if(!is_handled) {
            if(srcFilePath.compare(0, 4,"incl")) {
                dbgs() << "Unable to handle file path:" << srcFilePath << "\n";
            }
        }

        return to_ret;
    }
}

