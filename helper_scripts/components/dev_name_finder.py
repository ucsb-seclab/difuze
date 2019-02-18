from base_component import *
import os
from multiprocessing import Pool, cpu_count
import tempfile


class DevNameFinder(Component):
    """
        Component which tries to find device name for all the driver entry points.
    """

    def __init__(self, value_dict):
        dev_name_finder_so = None
        entry_point_out = None
        ioctl_finder_out = None
        opt_bin_path = None
        if 'dev_name_finder_so' in value_dict:
            dev_name_finder_so = value_dict['dev_name_finder_so']
        if 'entry_point_out' in value_dict:
            entry_point_out = value_dict['entry_point_out']
        if 'ioctl_finder_out' in value_dict:
            ioctl_finder_out = value_dict['ioctl_finder_out']
        if 'opt_bin_path' in value_dict:
            opt_bin_path = value_dict['opt_bin_path']

        self.opt_bin_path = opt_bin_path
        self.dev_name_finder_so = dev_name_finder_so
        self.entry_point_out = entry_point_out
        self.ioctl_finder_out = ioctl_finder_out

    def setup(self):
        if not os.path.exists(self.dev_name_finder_so):
            return "Provided dev name finder so path:" + str(self.dev_name_finder_so) + " does not exist."
        if not os.path.exists(self.opt_bin_path):
            return "Provided opt bin path:" + str(self.opt_bin_path) + " does not exist."
        if not os.path.exists(self.entry_point_out):
            return "Provided entry point out file path:" + str(self.entry_point_out) + " is invalid."
        if self.ioctl_finder_out is None:
            return "Provided ioctl cmd finder out file path:" + str(self.ioctl_finder_out) + " is invalid."
        # set up the directory if this is not present.
        if not os.path.isdir(self.ioctl_finder_out):
            os.system('mkdir -p ' + self.ioctl_finder_out)

        return None

    def perform(self):
        log_info("Invoking Dev Name finder")
        v4l2_ioctl_out = os.path.join(self.ioctl_finder_out, "v4l2_ioctl_out")
        os.system('mkdir -p ' + v4l2_ioctl_out)
        return _run_dev_name_finder_all(self.entry_point_out, self.opt_bin_path, self.dev_name_finder_so,
                                        self.ioctl_finder_out)

    def get_name(self):
        return "DevNameFinder"

    def is_critical(self):
        # Yes, this component is critical.
        return True


def _run_dev_name_finder(combined_arg):

    opt_bin_path = combined_arg[0]
    so_path = combined_arg[1]
    func_name = combined_arg[2]
    llvm_bc_file = combined_arg[3]
    output_file = combined_arg[4]
    temp_bc_file = tempfile.NamedTemporaryFile(delete=False)
    bc_file_name = temp_bc_file.name
    temp_bc_file.close()
    # run mem2reg
    ret_val = os.system(opt_bin_path + " -mem2reg " + llvm_bc_file + " -o " + bc_file_name)
    if ret_val != 0:
        log_error("LLVM mem2reg failed on:", llvm_bc_file, " for function:", func_name,
                  ", So the output you get may be wrong.")

    # Old ioctl cmd parser
    ret_val = os.system("timeout 15m " + opt_bin_path + " -analyze -debug -load " + so_path + ' -dev-name-finder -ioctlFunction=\"' +
                        str(func_name) + '\" ' + bc_file_name + ' >> ' + output_file + ' 2>&1')
    return ret_val, func_name


def _run_dev_name_finder_all(entry_point_out, opt_bin_path, ioctl_so_path, ioctl_finder_out):
    to_run_cmds = []
    fp = open(entry_point_out, 'r')
    all_lines = fp.readlines()
    fp.close()
    processed_func = []
    v4l2_ioctl_out = os.path.join(ioctl_finder_out, "v4l2_ioctl_out")
    for curr_ep in all_lines:
        curr_ep = curr_ep.strip()
        all_p = curr_ep.split(':')
        if all_p[0] == 'IOCTL' and (all_p[1] + all_p[2]) not in processed_func:
            processed_func.append(all_p[1] + all_p[2])
            to_run_cmds.append((opt_bin_path, ioctl_so_path, all_p[1], all_p[-1],
                                os.path.join(ioctl_finder_out, all_p[3])))
        if all_p[0] == 'V4IOCTL' and (all_p[1] + all_p[3]) not in processed_func:
            processed_func.append(all_p[1] + all_p[3])
            to_run_cmds.append((opt_bin_path, ioctl_so_path, all_p[1], all_p[-1],
                                os.path.join(v4l2_ioctl_out, all_p[4])))

    log_info("Found:", len(to_run_cmds), " ioctl functions to process.")
    log_info("Processing in multiprocessing mode")
    p = Pool(cpu_count())
    return_vals = p.map(_run_dev_name_finder, to_run_cmds)
    log_info("Finished processing:", len(to_run_cmds), " ioctl functions.")
    total_failed = 0
    for curr_r_val in return_vals:
        if int(curr_r_val[0]) != 0:
            total_failed += 1
            log_error("Dev name finder failed for:", curr_r_val[-1])
    log_info("Dev name finder failed for:", total_failed, " out of:", len(to_run_cmds), " Ioctl functions.")
    return True
