//
// Created by machiry on 1/30/17.
//

#include <iostream>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>

using namespace std;
using namespace llvm;

#define NETDEV_IOCTL "NETDEVIOCTL"
#define READ_HDR "FileRead"
#define WRITE_HDR "FileWrite"
#define IOCTL_HDR "IOCTL"
#define DEVATTR_SHOW "DEVSHOW"
#define DEVATTR_STORE "DEVSTORE"
#define V4L2_IOCTL_FUNC "V4IOCTL"
#define END_HDR "ENDEND"

typedef struct {
    std::string st_name;
    long mem_id;
    std::string method_lab;
} INT_STS;

INT_STS kernel_sts[] {
        {"struct.watchdog_ops", 9, IOCTL_HDR},
        {"struct.bin_attribute", 3, READ_HDR},
        {"struct.bin_attribute", 4, WRITE_HDR},
        {"struct.atmdev_ops", 3, IOCTL_HDR},
        {"struct.atmphy_ops", 1, IOCTL_HDR},
        {"struct.atm_ioctl", 1, IOCTL_HDR},
        {"struct.kernfs_ops", 4, READ_HDR},
        {"struct.kernfs_ops", 6, WRITE_HDR},
        {"struct.ppp_channel_ops", 1, IOCTL_HDR},
        {"struct.hdlcdrv_ops", 4, IOCTL_HDR},
        {"struct.vfio_device_ops", 3, READ_HDR},
        {"struct.vfio_device_ops", 4, WRITE_HDR},
        {"struct.vfio_device_ops", 5, IOCTL_HDR},
        {"struct.vfio_iommu_driver_ops", 4, READ_HDR},
        {"struct.vfio_iommu_driver_ops", 5, WRITE_HDR},
        {"struct.vfio_iommu_driver_ops", 6, IOCTL_HDR},
        {"struct.rtc_class_ops", 2, IOCTL_HDR},
        {"struct.net_device_ops", 10, IOCTL_HDR},
        {"struct.kvm_device_ops", 6, IOCTL_HDR},
        {"struct.ide_disk_ops", 8, IOCTL_HDR},
        {"struct.ide_ioctl_devset", 0, IOCTL_HDR},
        {"struct.ide_ioctl_devset", 1, IOCTL_HDR},
        {"struct.pci_ops", 0, READ_HDR},
        {"struct.pci_ops", 1, WRITE_HDR},
        {"struct.cdrom_device_info", 10, IOCTL_HDR},
        {"struct.cdrom_device_ops", 12, IOCTL_HDR},
        {"struct.iio_chan_spec_ext_info", 2, READ_HDR},
        {"struct.iio_chan_spec_ext_info", 3, WRITE_HDR},
        {"struct.proto_ops", 9, IOCTL_HDR},
        {"struct.usb_phy_io_ops", 0, READ_HDR},
        {"struct.usb_phy_io_ops", 1, WRITE_HDR},
        {"struct.usb_gadget_ops", 6, IOCTL_HDR},
        {"struct.uart_ops", 23, IOCTL_HDR},
        {"struct.tty_ldisc_ops", 8, READ_HDR},
        {"struct.tty_ldisc_ops", 9, WRITE_HDR},
        {"struct.tty_ldisc_ops", 10, IOCTL_HDR},
        {"struct.fb_ops", 17, IOCTL_HDR},
        {"struct.v4l2_subdev_core_ops", 13, IOCTL_HDR},
        {"struct.m2m1shot_devops", 8, IOCTL_HDR},
        {"struct.nfc_phy_ops", 0, WRITE_HDR},
        {"struct.snd_ac97_bus_ops", 2, WRITE_HDR},
        {"struct.snd_ac97_bus_ops", 3, READ_HDR},
        {"struct.snd_hwdep_ops", 1, READ_HDR},
        {"struct.snd_hwdep_ops", 2, WRITE_HDR},
        {"struct.snd_hwdep_ops", 6, IOCTL_HDR},
        {"struct.snd_hwdep_ops", 7, IOCTL_HDR},
        {"struct.snd_soc_component", 14, READ_HDR},
        {"struct.snd_soc_component", 15, WRITE_HDR},
        {"struct.snd_soc_codec_driver", 14, READ_HDR},
        {"struct.snd_soc_codec_driver", 15, WRITE_HDR},
        {"struct.snd_pcm_ops", 2, IOCTL_HDR},
        {"struct.snd_ak4xxx_ops", 2, WRITE_HDR},
        {"struct.snd_info_entry_text", 0, READ_HDR},
        {"struct.snd_info_entry_text", 1, WRITE_HDR},
        {"struct.snd_info_entry_ops", 2, READ_HDR},
        {"struct.snd_info_entry_ops", 3, WRITE_HDR},
        {"struct.snd_info_entry_ops", 6, IOCTL_HDR},
        {"struct.tty_buffer", 4, READ_HDR},
        {"struct.tty_operations", 7, WRITE_HDR},
        {"struct.tty_operations", 12, IOCTL_HDR},
        {"struct.posix_clock_operations", 10, IOCTL_HDR},
        {"struct.posix_clock_operations", 15, READ_HDR},
        {"struct.block_device_operations", 3, IOCTL_HDR},
        {"struct.security_operations", 64, IOCTL_HDR},
        {"struct.file_operations", 2, READ_HDR},
        {"struct.file_operations", 3, WRITE_HDR},
        {"struct.file_operations", 10, IOCTL_HDR},
        {"struct.efi_pci_io_protocol_access_32_t", 0, READ_HDR},
        {"struct.efi_pci_io_protocol_access_32_t", 1, WRITE_HDR},
        {"struct.efi_pci_io_protocol_access_64_t", 0, READ_HDR},
        {"struct.efi_pci_io_protocol_access_64_t", 1, WRITE_HDR},
        {"struct.efi_pci_io_protocol_access_t", 0, READ_HDR},
        {"struct.efi_pci_io_protocol_access_t", 1, WRITE_HDR},
        {"struct.efi_file_handle_32_t", 4, READ_HDR},
        {"struct.efi_file_handle_32_t", 5, WRITE_HDR},
        {"struct.efi_file_handle_64_t", 4, READ_HDR},
        {"struct.efi_file_handle_64_t", 5, WRITE_HDR},
        {"struct._efi_file_handle", 4, READ_HDR},
        {"struct._efi_file_handle", 5, WRITE_HDR},
        {"struct.video_device", 21, IOCTL_HDR},
        {"struct.video_device", 22, IOCTL_HDR},
        {"struct.v4l2_file_operations", 1, READ_HDR},
        {"struct.v4l2_file_operations", 2, WRITE_HDR},
        {"struct.v4l2_file_operations", 4, IOCTL_HDR},
        {"struct.v4l2_file_operations", 5, IOCTL_HDR},
        {"struct.media_file_operations", 1, READ_HDR},
        {"struct.media_file_operations", 2, WRITE_HDR},
        {"struct.media_file_operations", 4, IOCTL_HDR},
        {"END", 0, END_HDR}
};

void print_err(char *prog_name) {
    std::cerr << "[!] This program identifies all the entry points from the provided bitcode file.\n";
    std::cerr << "[!] saves these entry points into provided output file, which could be used to run analysis on.\n";
    std::cerr << "[?] " << prog_name << " <llvm_linked_bit_code_file> <output_txt_file>\n";
    exit(-1);
}

std::string getFunctionFileName(Function *targetFunc) {
    SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
    targetFunc->getAllMetadata(MDs);
    std::string targetFName = "";
    for (auto &MD : MDs) {
        if (MDNode *N = MD.second) {
            if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                targetFName = subProgram->getFilename();
            }
        }
    }
    return targetFName;
}


bool printFuncVal(Value *currVal, FILE *outputFile, const char *hdr_str) {
    Function *targetFunction = dyn_cast<Function>(currVal->stripPointerCasts());
    if (targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName()) {
        fprintf(outputFile, "%s:%s\n", hdr_str, targetFunction->getName().str().c_str());
        return true;
    }
    return false;
}

bool printTriFuncVal(Value *currVal, FILE *outputFile, const char *hdr_str) {
    Function *targetFunction = dyn_cast<Function>(currVal->stripPointerCasts());
    if (targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName()) {
        fprintf(outputFile, "%s:%s:%s\n", hdr_str, targetFunction->getName().str().c_str(),
                            getFunctionFileName(targetFunction).c_str());
        return true;
    }
    return false;
}

void process_netdev_st(GlobalVariable *currGlobal, FILE *outputFile) {

    if(currGlobal->hasInitializer()) {
        // get the initializer.
        Constant *targetConstant = currGlobal->getInitializer();
        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
        if(actualStType != nullptr) {
            // net device ioctl: 10
            if (actualStType->getNumOperands() > 10) {
                printFuncVal(actualStType->getOperand(10), outputFile, NETDEV_IOCTL);
            }
        }
    }
}

void process_device_attribute_st(GlobalVariable *currGlobal, FILE *outputFile) {

    if(currGlobal->hasInitializer()) {

        // get the initializer.
        Constant *targetConstant = currGlobal->getInitializer();
        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
        if(actualStType != nullptr) {
            if (actualStType->getNumOperands() > 1) {
                // dev show: 1
                printFuncVal(actualStType->getOperand(1), outputFile, DEVATTR_SHOW);
            }

            if (actualStType->getNumOperands() > 2) {
                // dev store : 2
                printFuncVal(actualStType->getOperand(2), outputFile, DEVATTR_STORE);
            }
        }

    }
}

inline bool ends_with(std::string const &value, std::string const &ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void process_file_operations_st(GlobalVariable *currGlobal, FILE *outputFile) {

    if(currGlobal->hasInitializer()) {

        // get the initializer.
        Constant *targetConstant = currGlobal->getInitializer();
        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
        bool ioctl_found = false, ioctl_found2 = false;
        if(actualStType != nullptr) {
            Value *currFieldVal;

            // read: 2
            if (actualStType->getNumOperands() > 2) {
                printFuncVal(actualStType->getOperand(2), outputFile, READ_HDR);
            }

            // write: 3
            if (actualStType->getNumOperands() > 3) {
                printFuncVal(actualStType->getOperand(3), outputFile, WRITE_HDR);
            }

            // ioctl : 10
            if (actualStType->getNumOperands() > 10) {
                ioctl_found = printTriFuncVal(actualStType->getOperand(10), outputFile, IOCTL_HDR);
            }

            // ioctl : 10
            if (actualStType->getNumOperands() > 8) {
                ioctl_found2 = printTriFuncVal(actualStType->getOperand(8), outputFile, IOCTL_HDR);
            }
            
            // ioctl function identification heuristic
            if(!ioctl_found || !ioctl_found2) {
                unsigned int idx=0;
                std::string ioctlEnd = "_ioctl";
                for(idx=0; idx<actualStType->getNumOperands(); idx++) {
                    if(idx == 10 || idx == 8) {
                        continue;
                    }
                    currFieldVal = actualStType->getOperand(idx);
                    Function *targetFunction = dyn_cast<Function>(currFieldVal->stripPointerCasts());
                    if(targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName() && ends_with(targetFunction->getName().str(), ioctlEnd)) {
                        printTriFuncVal(currFieldVal, outputFile, IOCTL_HDR);
                    }
                }
            }
        }

    }
}

void process_snd_pcm_ops_st(GlobalVariable *currGlobal, FILE *outputFile) {
    if(currGlobal->hasInitializer()) {
        // get the initializer.
        Constant *targetConstant = currGlobal->getInitializer();
        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
        if(actualStType != nullptr) {
            if (actualStType->getNumOperands() > 2) {
                // ioctl: 2
                printTriFuncVal(actualStType->getOperand(2), outputFile, IOCTL_HDR);
            }
        }

    }
}

void process_v4l2_ioctl_st(GlobalVariable *currGlobal, FILE *outputFile) {
    if(currGlobal->hasInitializer()) {

        // get the initializer.
        Constant *targetConstant = currGlobal->getInitializer();
        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
        if(actualStType != nullptr) {
            // all fields are function pointers
            for (unsigned int i = 0; i < actualStType->getNumOperands(); i++) {
                Value *currFieldVal = actualStType->getOperand(i);
                Function *targetFunction = dyn_cast<Function>(currFieldVal);
                if (targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName()) {
                    fprintf(outputFile, "%s:%s:%u:%s\n", V4L2_IOCTL_FUNC, targetFunction->getName().str().c_str(), i,
                            getFunctionFileName(targetFunction).c_str());
                }
            }
        }

    }
}

void process_v4l2_file_ops_st(GlobalVariable *currGlobal, FILE *outputFile) {
    if(currGlobal->hasInitializer()) {
        // get the initializer.
        Constant *targetConstant = currGlobal->getInitializer();
        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
        if(actualStType != nullptr) {
            // read: 1
            if (actualStType->getNumOperands() > 1) {
                printFuncVal(actualStType->getOperand(1), outputFile, READ_HDR);
            }

            // write: 2
            if (actualStType->getNumOperands() > 2) {
                printFuncVal(actualStType->getOperand(2), outputFile, WRITE_HDR);
            }
        }

    }
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

bool process_struct_in_custom_entry_files(GlobalVariable *currGlobal, FILE *outputFile,
                                          std::vector<string> &allentries) {
    bool retVal = false;
    if(currGlobal->hasInitializer()) {
        // get the initializer.
        Constant *targetConstant = currGlobal->getInitializer();
        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
        if(actualStType != nullptr) {
            Type *targetType = currGlobal->getType();
            assert(targetType->isPointerTy());
            Type *containedType = targetType->getContainedType(0);
            std::string curr_st_name = containedType->getStructName();
            char hello_str[1024];
            for (auto curre:allentries) {
                if (curre.find(curr_st_name) != std::string::npos) {
                    strcpy(hello_str, curre.c_str());
                    // structure found
                    char **tokens = str_split(hello_str, ',');
                    assert(!strcmp(curr_st_name.c_str(), tokens[0]));
                    long ele_ind = strtol(tokens[1], NULL, 10);
                    if (actualStType->getNumOperands() > ele_ind) {
                        Value *currFieldVal = actualStType->getOperand(ele_ind);
                        Function *targetFunction = dyn_cast<Function>(currFieldVal);

                        if (targetFunction != nullptr && !targetFunction->isDeclaration() &&
                            targetFunction->hasName()) {
                            fprintf(outputFile, "%s:%s:%s\n", tokens[2], targetFunction->getName().str().c_str(),
                                    getFunctionFileName(targetFunction).c_str());
                        }
                    }
                    if (tokens) {
                        int i;
                        for (i = 0; *(tokens + i); i++) {
                            free(*(tokens + i));
                        }
                        free(tokens);
                    }
                    retVal = true;
                }
            }
        }

    }
    return retVal;
}

void process_global(GlobalVariable *currGlobal, FILE *outputFile, std::vector<string> &allentries) {
    std::string file_op_st("struct.file_operations");
    std::string dev_attr_st("struct.device_attribute");
    std::string dri_attr_st("struct.driver_attribute");
    std::string bus_attr_st("struct.bus_attribute");
    std::string net_dev_st("struct.net_device_ops");
    std::string snd_pcm_st("struct.snd_pcm_ops");
    std::string v4l2_ioctl_st("struct.v4l2_ioctl_ops");
    std::string v4l2_file_ops_st("struct.v4l2_file_operations");


    Type *targetType = currGlobal->getType();
    assert(targetType->isPointerTy());
    Type *containedType = targetType->getContainedType(0);
    if (containedType->isStructTy()) {
        StructType *targetSt = dyn_cast<StructType>(containedType);
        if(targetSt->isLiteral()) {
            return;
        }
        if(process_struct_in_custom_entry_files(currGlobal, outputFile, allentries)) {
            return;
        }
        if (containedType->getStructName() == file_op_st) {
            process_file_operations_st(currGlobal, outputFile);
        } else if(containedType->getStructName() == dev_attr_st || containedType->getStructName() == dri_attr_st) {
            process_device_attribute_st(currGlobal, outputFile);
        } else if(containedType->getStructName() == net_dev_st) {
            process_netdev_st(currGlobal, outputFile);
        } else if(containedType->getStructName() == snd_pcm_st) {
            process_snd_pcm_ops_st(currGlobal, outputFile);
        } else if(containedType->getStructName() == v4l2_file_ops_st) {
            process_v4l2_file_ops_st(currGlobal, outputFile);
        } else if(containedType->getStructName() == v4l2_ioctl_st) {
            process_v4l2_ioctl_st(currGlobal, outputFile);
        } else {
            unsigned long i=0;
            while(kernel_sts[i].method_lab != END_HDR) {
                if(kernel_sts[i].st_name == containedType->getStructName()) {
                    if(currGlobal->hasInitializer()) {
                        Constant *targetConstant = currGlobal->getInitializer();
                        ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
                        Value *currFieldVal = actualStType->getOperand(kernel_sts[i].mem_id);
                        Function *targetFunction = dyn_cast<Function>(currFieldVal);

                        if(targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName()) {
                            fprintf(outputFile, "%s:%s:%s\n", kernel_sts[i].method_lab.c_str(),
                                    targetFunction->getName().str().c_str(), getFunctionFileName(targetFunction).c_str());
                        }
                    }
                }
                i++;
            }
        }
    }
}

// some debug function
/*void check_my_guy(Module *m, CallInst *currInst) {
    errs() << "Called for:" << *currInst << ":" << *(currInst->getFunctionType()) << "\n";
    for(auto a = m->begin(), b = m->end(); a != b; a++) {
        Function *my_func = &(*a);
        if (my_func != nullptr && !my_func->isDeclaration() && my_func->getFunctionType() == currInst->getFunctionType()) {
            errs() << my_func->getName() << ":" << my_func->getNumUses() << "\n";
            for (Value::user_iterator i = my_func->user_begin(), e = my_func->user_end(); i != e; ++i) {
                CallInst *currC = dyn_cast<CallInst>(*i);
                if (currC == nullptr) {
                    errs() << "F is used in a non call instruction:\n";
                    errs() << **i << "\n";
                }
            }

        }
    }
}*/

int main(int argc, char *argv[]) {
    //check args
    if(argc < 3) {
        print_err(argv[0]);
    }

    char *src_llvm_file = argv[1];
    char *output_txt_file = argv[2];
    char *entry_point_file = NULL;
    std::vector<string> entryPointStrings;
    entryPointStrings.clear();
    if(argc > 3) {
        entry_point_file = argv[3];
        std::ifstream infile(entry_point_file);
        std::string line;
        while (std::getline(infile, line)) {
            entryPointStrings.push_back(line);
        }
    }



    FILE *outputFile = fopen(output_txt_file, "w");
    assert(outputFile != nullptr);

    LLVMContext context;
    ErrorOr<std::unique_ptr<MemoryBuffer>> fileOrErr = MemoryBuffer::getFileOrSTDIN(src_llvm_file);

    ErrorOr<std::unique_ptr<llvm::Module>> moduleOrErr = parseBitcodeFile(fileOrErr.get()->getMemBufferRef(), context);

    Module *m = moduleOrErr.get().get();

    Module::GlobalListType &currGlobalList = m->getGlobalList();
    for(Module::global_iterator gstart = currGlobalList.begin(), gend = currGlobalList.end(); gstart != gend; gstart++) {
        GlobalVariable *currGlobal = &(*gstart);
        process_global(currGlobal, outputFile, entryPointStrings);
    }
    /*for(auto a = m->begin(), b = m->end(); a != b; a++) {
        //check_my_guy(&(*a));
        for(auto fi = (*a).begin(), fe = (*a).end(); fi != fe; fi++) {
            for(auto bs=(*fi).begin(), be=(*fi).end(); bs!=be; bs++) {
                Instruction *currI = &(*bs);
                assert(currI != nullptr);
                CallInst *currC = dyn_cast<CallInst>(currI);
                if(currC != nullptr && currC->getCalledFunction() == nullptr) {
                    check_my_guy(m, currC);
                }
            }
        }
    }*/
    fclose(outputFile);
}
