from base_component import *
import os
import subprocess


class DriverLinker(Component):
    """
        Component that links the driver bitcode file with all the dependent bitcode files.
    """

    def __init__(self, value_dict):
        dr_link_bin = None
        llvm_bc_out = None
        chipset_number = None
        if 'llvm_bc_out' in value_dict:
            llvm_bc_out = value_dict['llvm_bc_out']
        if 'dr_link_bin' in value_dict:
            dr_link_bin = value_dict['dr_link_bin']
        if 'chipset_num' in value_dict:
            chipset_number = int(value_dict['chipset_num'])
        self.llvm_bc_out = llvm_bc_out
        self.dr_link_bin = dr_link_bin
        self.chipset_numer = chipset_number

    def setup(self):
        if not os.path.exists(self.llvm_bc_out) or not os.path.isdir(self.llvm_bc_out):
            return "Provided LLVM Bitcode directory:" + str(self.llvm_bc_out) + " does not exist."
        if not os.path.exists(self.dr_link_bin) or not os.path.isfile(self.dr_link_bin):
            return "Provided dr link bin path does not exist:" + str(self.dr_link_bin)
        if self.chipset_numer is None or self.chipset_numer < 1 or self.chipset_numer > 4:
            return "Invalid chipset number. Valid chipset numbers are: 1(mediatek)|2(qualcomm)|3(huawei)|4(samsung)"
        return None

    def perform(self):
        log_info("Running dr_linker. This might take time. Please wait.")
        cmd_to_run = self.dr_link_bin + " " + self.llvm_bc_out + " " + str(self.chipset_numer)
        returncode = os.system(cmd_to_run)
        '''p = subprocess.Popen(self.dr_link_bin + " " + self.llvm_bc_out + " " + str(self.chipset_numer), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             shell=True)
        (stdout, stderr) = p.communicate()'''
        log_info("dr_linker finished execution.")
        if returncode == 0:
            log_info("Running llvm-link to generate the final linked bitcode file.")
            return _process_dir(self.llvm_bc_out)
        else:
            log_error("Error occurred while executing:", cmd_to_run)
            return False

    def get_name(self):
        return "DrLinker"

    def is_critical(self):
        # Yes, this component is critical
        return True


def _get_all_link_folders(curr_folder):
    dst_folders = []
    for curr_file in os.listdir(curr_folder):
        file_fullpath = os.path.join(curr_folder, curr_file)
        if curr_file == "llvm_link_out":
            dst_folders.append(file_fullpath)
        elif os.path.isdir(file_fullpath):
            child_dir = _get_all_link_folders(file_fullpath)
            dst_folders.extend(child_dir)
        else:
            pass
    return dst_folders


def _is_bit_code_file(curr_file):
    fp = open(curr_file, 'r')
    conts = fp.read(2)
    fp.close()
    return conts == 'BC'


def _get_all_files(curr_folder):
    to_ret = []
    for cf in os.listdir(curr_folder):
        cfp = os.path.join(curr_folder, cf)
        if os.path.isfile(cfp) and _is_bit_code_file(cfp):
            to_ret.append(cfp)
        if os.path.isdir(cfp):
            child_files = _get_all_files(cfp)
            to_ret.extend(child_files)
    return to_ret


def _process_dir(curr_folder):
    to_check_folder = os.path.join(curr_folder, "llvm_link_out")
    if os.path.isdir(to_check_folder):
        all_link_files = _get_all_files(curr_folder)
        if len(all_link_files) > 0:
            final_output_dir = os.path.join(curr_folder, "llvm_link_final")
            os.system('mkdir -p ' + final_output_dir)
            log_info("To link folder:" + final_output_dir)
            for cu_d in all_link_files:
                os.system('cp ' + cu_d + ' ' + final_output_dir)
            to_test_bc = os.path.join(final_output_dir, "final_to_check.bc")
            os.system('llvm-link ' +
                      final_output_dir + "/* -o " + to_test_bc)
            log_success("To check BC file:" + to_test_bc)
    # Do not stop when a llvm_link_out is found.
    # Continue finding in child directories.
    for cdir_name in os.listdir(curr_folder):
        cdir_path = os.path.join(curr_folder, cdir_name)
        if os.path.isdir(cdir_path):
            _process_dir(cdir_path)
    return True
