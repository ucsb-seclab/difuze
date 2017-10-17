from base_component import *
import os
import xml.etree.ElementTree as ET


class ParseV4L2Headers(Component):
    """
        Component which parses the v4l2 headers to get ioctl function pointer structure members.
    """

    def __init__(self, value_dict):
        c2xml_path = None
        kernel_src_dir = None
        makeout_file = None
        separate_out = None
        v4l2_func_list_file = None
        v4l2_id_cmd_out = None
        opt_bin_path = None
        llvm_bc_out = ""
        v4l2_config_processor_so = None
        if 'c2xml_bin' in value_dict:
            c2xml_path = value_dict['c2xml_bin']
        if 'kernel_src_dir' in value_dict:
            kernel_src_dir = value_dict['kernel_src_dir']
        if 'makeout' in value_dict:
            makeout_file = value_dict['makeout']
        if 'out' in value_dict:
            separate_out = value_dict['out']
        if 'v4l2_func_list' in value_dict:
            v4l2_func_list_file = value_dict['v4l2_func_list']
        if 'llvm_bc_out' in value_dict:
            llvm_bc_out = value_dict["llvm_bc_out"]
        if 'v4l2_id_cmd_out' in value_dict:
            v4l2_id_cmd_out = value_dict['v4l2_id_cmd_out']
        if 'opt_bin_path' in value_dict:
            opt_bin_path = value_dict['opt_bin_path']
        if 'v4l2_config_processor_so' in value_dict:
            v4l2_config_processor_so = value_dict['v4l2_config_processor_so']
        self.kernel_src_dir = kernel_src_dir
        self.c2xml_path = c2xml_path
        self.makeout_file = makeout_file
        self.separate_out = separate_out
        self.v4l2_func_list_file = v4l2_func_list_file
        self.v4l2_id_cmd_out = v4l2_id_cmd_out
        self.opt_bin_path = opt_bin_path
        self.llvm_bc_out = llvm_bc_out
        self.v4l2_config_processor_so = v4l2_config_processor_so

    def setup(self):
        """
            Perform setup.
        :return: Error msg or none
        """
        if not os.path.exists(self.v4l2_config_processor_so):
            return "Provided v4l2 config processor so path:" + str(self.v4l2_config_processor_so) + " does not exist."
        if not os.path.exists(self.c2xml_path):
            return "Provided c2xml path:" + str(self.c2xml_path) + " does not exist."
        if not os.path.isdir(self.kernel_src_dir) or not os.path.isdir(os.path.join(self.kernel_src_dir, 'include')):
            return "Provided kernel src directory is invalid. " \
                   "The base directory is not present or it does not contain include folder"
        if self.v4l2_func_list_file is None:
            return "No file specified to output v4l2 func list."
        if not os.path.exists(self.opt_bin_path):
            return "Provided opt bin path:" + str(self.opt_bin_path) + " does not exist."
        if self.v4l2_id_cmd_out is None:
            return "No file specified to output v4l2 id -> cmdid list."
        return None

    def perform(self):
        """
            Parse the headers
        :return: True or False
        """

        v4l2_hdr_file = os.path.join(self.kernel_src_dir, "include/media/v4l2-ioctl.h")
        if os.path.exists(v4l2_hdr_file):
            log_success("Grep ran successfully to find ops and operations structures.")
            log_info("Running c2xml to find entry point configurations.")
            target_bc_file = os.path.join(self.llvm_bc_out, "drivers/media/v4l2-core/v4l2-ioctl.llvm.bc")
            if not os.path.exists(target_bc_file):
                log_error("Unable to find v4l2 base bitcode file:" + str(target_bc_file))
                return False
            # second, run c2xml on all the header files.
            if self.separate_out is None:
                self.separate_out = self.kernel_src_dir
            ret_val = _run_c2xml(self.c2xml_path, self.makeout_file, v4l2_hdr_file, self.v4l2_func_list_file,
                                 dst_work_dir=self.separate_out)
            if ret_val:
                ret_val = os.system(self.opt_bin_path + " -analyze -debug -load " + self.v4l2_config_processor_so +
                                    ' -v4l2-config-processor -v4l2config=\"' +
                                    self.v4l2_func_list_file + '\" -v4l2idconfig=\"' + self.v4l2_id_cmd_out + '\" ' +
                                    target_bc_file)
                return ret_val == 0
            return ret_val
            # if ret_val:


        else:
            log_error("Unable to find v4l2 hdr file:" + str(v4l2_hdr_file))

    def get_name(self):
        """
            get component name.
        :return: Str
        """
        return "ParseV4L2Headers"

    def is_critical(self):
        """
            This component is not critical.
        :return: False
        """
        return False


gcc_bins = ['aarch64-linux-android-gcc', 'arm-eabi-gcc']


def _is_comp_binary(arg_zero):
    global gcc_bins
    for curr_c in gcc_bins:
        if str(arg_zero).endswith(curr_c):
            return True
    return False


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


def _run_c2xml(c2xml_bin, makeout_file,  target_v4l2_hdr, output_file, dst_work_dir=None):
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
    all_hdr_files = [target_v4l2_hdr]

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
                if curr_s.get("type") == "struct" and curr_s.get("file") == curr_hdr_file and \
                                curr_s.get("ident") == "v4l2_ioctl_ops":
                    child_no = 0
                    for child_s in curr_s:
                        target_fun_name = child_s.get("ident")
                        output_fp.write(target_fun_name + "," + str(child_no) + "\n")
                        child_no += 1
        if dst_work_dir is not None:
            os.chdir(back_wd)
    output_fp.close()
    return True

