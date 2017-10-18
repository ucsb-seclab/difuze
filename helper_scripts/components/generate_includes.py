from base_component import *
import os


class GenerateIncludes(Component):
    """
        Component which generates all the include files
        required for compiling a particular file.
    """
    def __init__(self, value_dict):
        makeout_file = None
        llvm_bc_out = None
        compiler_name = None
        kernel_src_dir = ""
        if 'makeout' in value_dict:
            makeout_file = value_dict['makeout']
        if 'llvm_bc_out' in value_dict:
            llvm_bc_out = value_dict['llvm_bc_out']
        if 'compiler_name' in value_dict:
            compiler_name = value_dict['compiler_name']
        if 'kernel_src_dir' in value_dict:
            kernel_src_dir = value_dict["kernel_src_dir"]

        self.kernel_src_dir = kernel_src_dir
        self.curr_makeout = makeout_file
        self.llvm_bc_out = llvm_bc_out
        self.gcc_name = compiler_name

    def setup(self):
        if not os.path.exists(self.curr_makeout):
            return "Required files(" + str(self.curr_makeout) + ") do not exist"
        if self.llvm_bc_out is not None:
            if not os.path.exists(self.llvm_bc_out):
                os.makedirs(self.llvm_bc_out)
            if os.path.isfile(self.llvm_bc_out):
                return "Provided LLVM Bitcode out:" + self.llvm_bc_out + " is a file, but should be a directory path."
        else:
            return "LLVM Bitcode out should be provided."
        if self.gcc_name is None:
            return "No compiler name specified, this is needed to filter compilation lines"
        return None

    def perform(self):
        return _generate_includes(self.kernel_src_dir, self.llvm_bc_out, self.curr_makeout, self.gcc_name)

    def get_name(self):
        return "GenerateIncludes"

    def is_critical(self):
        # No, this component is not critical.
        return False

# UTILITIES FUNCTION


def _get_src_file(build_args):
    """
        Get source file from the build command line.
    :param build_args: list of build args
    :return: file being compiled.
    """
    return build_args[-1]


def _split_includes(src_root_dir, gcc_build_string, output_folder):
    """
        Get LLVM build string from gcc build string
    :param src_root_dir: Directory containing all sources.
    :param gcc_build_string: GCC build string.
    :param output_folder: folder where the .includes should be placed.
    """

    orig_build_args = gcc_build_string.strip().split()[1:]
    rel_src_file_name = _get_src_file(orig_build_args)

    src_file_name = os.path.basename(rel_src_file_name)

    if str(rel_src_file_name).startswith("../"):
        rel_src_file_name = rel_src_file_name[3:]
    if str(rel_src_file_name).startswith('/'):
        rel_src_file_name = os.path.abspath(rel_src_file_name)
        if src_root_dir[-1] == '/':
            rel_src_file_name = rel_src_file_name[len(src_root_dir):]
        else:
            rel_src_file_name = rel_src_file_name[len(src_root_dir) + 1:]
    # replace output file with llvm bc file
    src_dir_name = os.path.dirname(rel_src_file_name)

    curr_output_dir = os.path.join(output_folder, src_dir_name)
    os.system('mkdir -p ' + curr_output_dir)

    curr_output_file = os.path.abspath(os.path.join(curr_output_dir, src_file_name[:-2] + '.includes'))
    fp_out = open(curr_output_file, 'w')
    # OK, now get all flags that start with -I and out them in curr_output_file
    for curr_flag in orig_build_args:
        curr_flag = curr_flag.strip()
        if str(curr_flag).startswith('-I'):
            fp_out.write(curr_flag + '\n')
    fp_out.close()


def _generate_includes(kernel_src_dir, base_output_folder, makeout_file, gcc_prefix):
    """
    Generate includes from all compilation lines.
    :param kernel_src_dir: Directory containing the kernel sources.
    :param base_output_folder: output folder where all the includes file should be stored.
    :param makeout_file: makeout.txt containing all the compilation lines.
    :param gcc_prefix: compiler used to compile all the driver files.
    :return: True/false depending on whether this step succeded or not.
    """
    makeout_lines = open(makeout_file, 'r').readlines()
    for curr_line in makeout_lines:
        curr_line = curr_line.strip()
        if curr_line.startswith(gcc_prefix):
            _split_includes(kernel_src_dir, curr_line, base_output_folder)
        elif len(curr_line.split()) > 2 and curr_line.split()[1] == gcc_prefix:
            _split_includes(kernel_src_dir, ' '.join(curr_line.split()[1:]), base_output_folder)
    return True
