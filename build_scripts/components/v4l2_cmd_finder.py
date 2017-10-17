from base_component import *
import os
import argparse
from multiprocessing import Pool, cpu_count
import tempfile


class V4L2CmdFinder(Component):
    """
        Component which tries to find all v4l2 commands and their corresponding structure.
    """

    def __init__(self, value_dict):
        entry_point_out = None
        ioctl_finder_out = None
        opt_bin_path = None
        llvm_bc_out = ""
        kernel_src_dir = ""
        v4l2_config_processor_so = None
        v4l2_id_cmd_out = None
        if 'entry_point_out' in value_dict:
            entry_point_out = value_dict['entry_point_out']
        if 'ioctl_finder_out' in value_dict:
            ioctl_finder_out = value_dict['ioctl_finder_out']
        if 'opt_bin_path' in value_dict:
            opt_bin_path = value_dict['opt_bin_path']
        if 'llvm_bc_out' in value_dict:
            llvm_bc_out = value_dict["llvm_bc_out"]
        if 'kernel_src_dir' in value_dict:
            kernel_src_dir = value_dict["kernel_src_dir"]
        if 'v4l2_config_processor_so' in value_dict:
            v4l2_config_processor_so = value_dict['v4l2_config_processor_so']
        if 'v4l2_id_cmd_out' in value_dict:
            v4l2_id_cmd_out = value_dict['v4l2_id_cmd_out']

        self.opt_bin_path = opt_bin_path
        self.entry_point_out = entry_point_out
        self.ioctl_finder_out = ioctl_finder_out
        self.llvm_bc_out = llvm_bc_out
        self.kernel_src_dir = kernel_src_dir
        self.v4l2_config_processor_so = v4l2_config_processor_so
        self.v4l2_id_cmd_out = v4l2_id_cmd_out

    def setup(self):
        if not os.path.exists(self.v4l2_config_processor_so):
            return "Provided v4l2 config processor so path:" + str(self.v4l2_config_processor_so) + " does not exist."
        if not os.path.exists(self.opt_bin_path):
            return "Provided opt bin path:" + str(self.opt_bin_path) + " does not exist."
        if not os.path.exists(self.entry_point_out):
            return "Provided entry point out file path:" + str(self.entry_point_out) + " is invalid."
        if self.ioctl_finder_out is None or not os.path.isdir(self.ioctl_finder_out):
            return "Provided ioctl cmd finder out file path:" + str(self.ioctl_finder_out) + " is invalid."
        if not os.path.exists(self.v4l2_id_cmd_out):
            return "No file specified for v4l2 id -> cmdid list."

        return None

    def perform(self):
        log_info("Invoking V4L2CmdFinder")
        return _run_ioctl_cmd_finder(self.entry_point_out, self.opt_bin_path, self.v4l2_config_processor_so,
                                     self.ioctl_finder_out, self.llvm_bc_out, self.kernel_src_dir, self.v4l2_id_cmd_out)

    def get_name(self):
        return "V4L2CmdFinder"

    def is_critical(self):
        # Yes, this component is critical.
        return True


def setup_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('-e', action='store', dest='entry_point_out',
                        help='Path to the entry point output file.')

    parser.add_argument('-p', action='store', dest='opt_bin_path',
                        help='Path to the opt executable.')

    parser.add_argument('-s', action='store', dest='v4l2_config_processor_so',
                        help='Path to the v4l2 config processor shared object (so).')

    parser.add_argument('-f', action='store', dest='ioctl_finder_out',
                        help='Path to the output folder where the ioctl command finder output should be stored.')

    return parser


def main():
    arg_parser = setup_args()
    parsed_args = arg_parser.parse_args()
    arg_dict = dict()
    arg_dict['opt_bin_path'] = parsed_args.opt_bin_path
    arg_dict['v4l2_config_processor_so'] = parsed_args.v4l2_config_processor_so
    arg_dict['ioctl_finder_out'] = parsed_args.ioctl_finder_out
    arg_dict['entry_point_out'] = parsed_args.entry_point_out
    ioctl_comp_out = V4L2CmdFinder(arg_dict)
    setup_msg = ioctl_comp_out.setup()
    if setup_msg is not None:
        log_error("Component:", ioctl_comp_out.get_name(), " setup failed with msg:", setup_msg)
    else:
        perf_out = ioctl_comp_out


def _run_ioctl_cmd_parser(combined_arg):

    opt_bin_path = combined_arg[0]
    so_path = combined_arg[1]
    func_name = combined_arg[2]
    llvm_bc_file = combined_arg[3]
    output_file = combined_arg[4]
    llvm_bc_out = combined_arg[5]
    kernel_src_dir = combined_arg[6]
    target_v4l2_func_index = combined_arg[7]
    v4l2_id_cmd_out = combined_arg[8]
    temp_bc_file = tempfile.NamedTemporaryFile(delete=False)
    bc_file_name = temp_bc_file.name
    temp_bc_file.close()
    # run mem2reg
    ret_val = os.system(opt_bin_path + " -mem2reg " + llvm_bc_file + " -o " + bc_file_name)
    if ret_val != 0:
        log_error("LLVM mem2reg failed on:", llvm_bc_file, " for function:", func_name,
                  ", So the output you get may be wrong.")

    # Old ioctl cmd parser
    '''ret_val = os.system(opt_bin_path + " -analyze -debug -load " + so_path + ' -ioctl-cmd-parser -toCheckFunction=\"' +
                        str(func_name) + '\" ' + bc_file_name + ' > ' + output_file + ' 2>&1')'''
    ret_val = os.system(opt_bin_path + " -analyze -debug -load " + so_path + ' -v4l2-processor -ioctlFunction=\"' +
                        str(func_name) + '\" -bcOutDir=\"' + llvm_bc_out + '\" -srcBaseDir=\"' + kernel_src_dir +
                        '\" -v4l2indexconfig=\"' + v4l2_id_cmd_out + '\" -funcIndex=' + target_v4l2_func_index + ' ' +
                        bc_file_name + ' > ' + output_file + ' 2>&1')
    return ret_val, func_name


def _get_file_to_write(func_name, output_folder):
    output_file = os.path.join(output_folder, func_name + '.txt')
    func_num = 1
    while os.path.exists(output_file):
        output_file = os.path.join(output_folder, func_name + '_' + str(func_num) + '.txt')
        func_num += 1
    return output_file


def _run_ioctl_cmd_finder(entry_point_out, opt_bin_path, ioctl_so_path, ioctl_finder_out, llvm_bc_out, kernel_src_dir,
                          v4l2_id_cmd_out):
    to_run_cmds = []
    fp = open(entry_point_out, 'r')
    all_lines = fp.readlines()
    fp.close()
    processed_func = []
    ioctl_finder_out = os.path.join(ioctl_finder_out, "v4l2_ioctl_out")
    os.system('mkdir -p ' + ioctl_finder_out)
    for curr_ep in all_lines:
        curr_ep = curr_ep.strip()
        all_p = curr_ep.split(':')
        if all_p[0] == 'V4IOCTL' and (all_p[1] + all_p[3]) not in processed_func:
            processed_func.append(all_p[1] + all_p[3])
            target_index = all_p[2]
            to_run_cmds.append((opt_bin_path, ioctl_so_path, all_p[1], all_p[-1],
                                os.path.join(ioctl_finder_out, all_p[4]), llvm_bc_out,
                                kernel_src_dir, target_index, v4l2_id_cmd_out))

    log_info("Found:", len(to_run_cmds), " v4l2 ioctl functions to process.")
    log_info("Processing in multiprocessing mode")
    p = Pool(cpu_count())
    return_vals = p.map(_run_ioctl_cmd_parser, to_run_cmds)
    log_info("Finished processing:", len(to_run_cmds), " ioctl functions.")
    total_failed = 0
    for curr_r_val in return_vals:
        if int(curr_r_val[0]) != 0:
            total_failed += 1
            log_error("V4l2 Ioctl cmd finder failed for:", curr_r_val[-1])
    log_info("V4l2 Ioctl Cmd finder failed for:", total_failed, " out of:", len(to_run_cmds), " Ioctl functions.")
    return True


if __name__ == "__main__":
    main()
