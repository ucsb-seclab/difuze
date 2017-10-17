from base_component import *
import os
import random
import xml.etree.ElementTree as ET


class ParseHeaders(Component):
    """
        Component which parses the kernel headers to get ioctl function pointer structure members.
    """

    def __init__(self, value_dict):
        c2xml_path = None
        kernel_src_dir = None
        makeout_file = None
        separate_out = None
        hdr_file_list = None
        if 'c2xml_bin' in value_dict:
            c2xml_path = value_dict['c2xml_bin']
        if 'kernel_src_dir' in value_dict:
            kernel_src_dir = value_dict['kernel_src_dir']
        if 'makeout' in value_dict:
            makeout_file = value_dict['makeout']
        if 'out' in value_dict:
            separate_out = value_dict['out']
        if 'hdr_file_list' in value_dict:
            hdr_file_list = value_dict['hdr_file_list']
        self.kernel_src_dir = kernel_src_dir
        self.c2xml_path = c2xml_path
        self.makeout_file = makeout_file
        self.separate_out = separate_out
        self.hdr_file_list = hdr_file_list

    def setup(self):
        """
            Perform setup.
        :return: Error msg or none
        """
        if not os.path.exists(self.c2xml_path):
            return "Provided c2xml path:" + str(self.c2xml_path) + " does not exist."
        if not os.path.isdir(self.kernel_src_dir) or not os.path.isdir(os.path.join(self.kernel_src_dir, 'include')):
            return "Provided kernel src directory is invalid. " \
                   "The base directory is not present or it does not contain include folder:" + \
                   str(self.kernel_src_dir) + ", " + os.path.join(self.kernel_src_dir, 'include')
        if self.hdr_file_list is None:
            return "No file specified to output hdr file list."
        return None

    def perform(self):
        """
            Parse the headers
        :return: True or False
        """
        temp_hdr_file_list = '/tmp/' + str(random.randint(1, 200000)) + '_hdrs.txt'
        kernel_include_dir = os.path.join(self.kernel_src_dir, "include")
        log_info("Running grep to find ops and operations structure.")
        # first, run grep to find all the header files.
        os.system("grep -rl \'operations {\' " + str(kernel_include_dir) + " > " + str(temp_hdr_file_list))
        os.system("grep -rl \'ops {\' " + str(kernel_include_dir) + " >> " + str(temp_hdr_file_list))
        output_file_size = 0
        if os.path.exists(temp_hdr_file_list):
            sti = os.stat(temp_hdr_file_list)
            output_file_size = sti.st_size
        if output_file_size > 0:
            log_success("Grep ran successfully to find ops and operations structures.")
            log_info("Running c2xml to find entry point configurations.")
            # second, run c2xml on all the header files.
            if self.separate_out is None:
                self.separate_out = self.kernel_src_dir
            return _run_c2xml(self.c2xml_path, self.makeout_file, temp_hdr_file_list, self.hdr_file_list,
                              dst_work_dir=self.separate_out)

        else:
            log_error("Grep failed, found no header files. Will be using default structures.")

    def get_name(self):
        """
            get component name.
        :return: Str
        """
        return "ParseHeaders"

    def is_critical(self):
        """
            This component is not critical.
        :return: False
        """
        return False


gcc_bins = ['aarch64-linux-android-gcc', 'arm-eabi-gcc']


def _handle_compile_command(comp_str, dst_includes):
    comp_args = comp_str.split()
    i = 0
    while i < len(comp_args):
        curr_arg = comp_args[i].strip()
        if curr_arg == "-isystem":
            curr_arg1 = "-I" + comp_args[i+1].strip()
            if curr_arg1 not in dst_includes:
                dst_includes.append(curr_arg1)
        if curr_arg == "-include":
            curr_arg1 = comp_args[i+1].strip()
            if "dhd_sec_feature.h" not in curr_arg1:
                final_arg = curr_arg + " " + curr_arg1
                if final_arg not in dst_includes:
                    dst_includes.append(final_arg)
        if curr_arg[0:2] == "-I":
            if curr_arg not in dst_includes:
                if 'drivers' not in curr_arg and 'sound' not in curr_arg:
                    dst_includes.append(curr_arg)
        i += 1


def _is_comp_binary(arg_zero):
    global gcc_bins
    for curr_c in gcc_bins:
        if str(arg_zero).endswith(curr_c):
            return True
    return False


def _run_c2xml(c2xml_bin, makeout_file, dst_hdr_file_list, output_file, dst_work_dir=None):
    fp = open(makeout_file, "r")
    all_comp_lines = fp.readlines()
    fp.close()

    all_hdr_options = list()
    all_hdr_options.append("-Iinclude")
    for comp_line in all_comp_lines:
        comp_line = comp_line.strip()
        comp_args = comp_line.split()
        if len(comp_args) > 2:
            if _is_comp_binary(comp_args[0]) or _is_comp_binary(comp_args[1]):
                _handle_compile_command(comp_line, all_hdr_options)

    all_hdr_options.append("-D__KERNEL__")
    all_hdr_files = []
    fp = open(dst_hdr_file_list, "r")
    all_hdr_files = fp.readlines()
    fp.close()

    dummy_out_file = "/tmp/dummy_out.xml"

    output_fp = open(output_file, "w")
    for curr_hdr_file in all_hdr_files:
        curr_hdr_file = curr_hdr_file.strip()
        cmd_line = " ".join(all_hdr_options)
        cmd_line = c2xml_bin + " " + cmd_line + " " + curr_hdr_file + " > " + dummy_out_file + " 2>/dev/null"
        back_wd = os.getcwd()
        if dst_work_dir is not None:
            os.chdir(dst_work_dir)
        # print "Running Command:" + cmd_line
        os.system(cmd_line)
        if os.path.exists(dummy_out_file) and os.stat(dummy_out_file).st_size > 0:
            root = ET.parse(dummy_out_file).getroot()
            for curr_s in root:
                if curr_s.get("type") == "struct" and curr_s.get("file") == curr_hdr_file:
                    child_no = 0
                    for child_s in curr_s:
                        if 'ioctl' in str(child_s.get("ident")) and 'compat_ioctl' not in child_s.get("ident"):
                            output_fp.write('struct.' + str(curr_s.get("ident")) + ',' + str(child_no) + ',IOCTL\n')
                        if child_s.get("ident") == "read":
                            output_fp.write('struct.' + str(curr_s.get("ident")) + ',' + str(child_no) + ',FileRead\n')
                        if child_s.get("ident") == "write":
                            output_fp.write('struct.' + str(curr_s.get("ident")) + ',' + str(child_no) + ',FileWrite\n')
                        child_no += 1
        if dst_work_dir is not None:
            os.chdir(back_wd)
    output_fp.close()
    return True

