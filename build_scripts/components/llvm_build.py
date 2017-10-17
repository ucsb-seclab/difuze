from base_component import *
import os
from multiprocessing import Pool, cpu_count


class LLVMBuild(Component):
    """
        Component which builds LLVM Bitcode files.
    """
    def __init__(self, value_dict):
        makeout_file = None
        clang_bin = None
        llvm_bc_out = None
        compiler_name = None
        arch_num = None
        separate_out = None
        kernel_src_dir = ""
        if 'makeout' in value_dict:
            makeout_file = value_dict['makeout']
        if 'clangbin' in value_dict:
            clang_bin = value_dict['clangbin']
        if 'llvm_bc_out' in value_dict:
            llvm_bc_out = value_dict['llvm_bc_out']
        if 'compiler_name' in value_dict:
            compiler_name = value_dict['compiler_name']
        if 'arch_num' in value_dict:
            arch_num = value_dict['arch_num']
        if 'out' in value_dict:
            separate_out = value_dict['out']
        if 'kernel_src_dir' in value_dict:
            kernel_src_dir = value_dict["kernel_src_dir"]

        self.curr_makeout = makeout_file
        self.clang_bin = clang_bin
        self.llvm_bc_out = llvm_bc_out
        self.gcc_name = compiler_name
        self.arch_num = arch_num
        self.separate_out = separate_out
        self.kernel_src_dir = kernel_src_dir

    def setup(self):
        if (not os.path.exists(self.curr_makeout)) or \
                (not os.path.exists(self.clang_bin)):
            return "Required files(" + str(self.curr_makeout) + ", " + str(self.clang_bin) + ") do not exist"
        if self.llvm_bc_out is not None:
            if not os.path.exists(self.llvm_bc_out):
                os.makedirs(self.llvm_bc_out)
            if os.path.isfile(self.llvm_bc_out):
                return "Provided LLVM Bitcode out:" + self.llvm_bc_out + " is a file, but should be a directory path."
        else:
            return "LLVM Bitcode out should be provided."
        if self.gcc_name is None:
            return "No compiler name specified, this is needed to filter compilation lines"
        if self.arch_num is None:
            return "No architecture number provided."
        return None

    def perform(self):
        return _generate_llvm_bitcode(self.kernel_src_dir, self.llvm_bc_out, self.curr_makeout, self.gcc_name,
                                      self.arch_num, self.clang_bin, command_output_dir=self.separate_out)

    def get_name(self):
        return "BuildLLVM"

    def is_critical(self):
        # Yes, this component is critical.
        return True

# UTILITIES FUNCTION
# These flags should be removed from gcc cmdline
INVALID_GCC_FLAGS = ['-mno-thumb-interwork', '-fconserve-stack', '-fno-var-tracking-assignments',
                     '-fno-delete-null-pointer-checks', '--param=allow-store-data-races=0',
                     '-Wno-unused-but-set-variable', '-Werror=frame-larger-than=1', '-Werror', '-Wall',
                     '-fno-jump-tables', '-nostdinc']
# target optimization to be used for llvm
TARGET_OPTIMIZATION_FLAGS = ['-O0']
# debug flags to be used by llvm
DEBUG_INFO_FLAGS = ['-g']
ARCH_TARGET = '-target'
# ARM 32 architecture flag for LLVM
ARM_32_LLVM_ARCH = 'armv7-a'
# ARM 64 architecture flag for LLVM
ARM_64_LLVM_ARCH = 'arm64'
# flags to disable some llvm warnings
DISABLE_WARNINGS = ['-Wno-return-type', '-w']
# flags for architecture
ARM_32 = 1
ARM_64 = 2
# path to the clang binary
CLANG_PATH = 'clang'
EMIT_LLVM_FLAG = '-emit-llvm'


def _run_program(cmd_to_run):
    """
        Run the provided command and return the corresponding return code.
    :param cmd_to_run: Command to run
    :return: Return value
    """
    return os.system(cmd_to_run)


def _get_src_file(build_args):
    """
        Get source file from the build command line.
    :param build_args: list of build args
    :return: file being compiled.
    """
    return build_args[-1]


def _get_output_file_idx(build_args):
    """
        Get the index of output file from build args
    :param build_args: arguments of gcc
    :return: index in the build args where output file is.
    """
    i = 0
    while i < len(build_args):
        if build_args[i] == "-o":
            return i + 1
        i += 1
    return -1


def _is_allowed_flag(curr_flag):
    """
        Function which checks, if a gcc flag is allowed in llvm command line.
    :param curr_flag: flag to include in llvm
    :return: True/False
    """
    # if this is a optimization flag, remove it.
    if str(curr_flag)[:2] == "-O":
        return False

    # if the flag is invalid
    for curr_in_flag in INVALID_GCC_FLAGS:
        if curr_flag.startswith(curr_in_flag):
            return False

    return True


def _get_llvm_build_str(src_root_dir, gcc_build_string, output_folder, target_arch, clang_path, build_output_dir=None):
    """
        Get LLVM build string from gcc build string
    :param src_root_dir: Directory containing all sources.
    :param gcc_build_string: GCC build string.
    :param output_folder: folder where llvm bitcode should be placed.
    :param target_arch: [1/2] depending on whether the arch is 32 or 64 bit.
    :param build_output_dir: Directory from which build commands need to be executed.
    :return: LLVM build string
    """

    orig_build_args = gcc_build_string.strip().split()[1:]
    rel_src_file_name = _get_src_file(orig_build_args)

    if build_output_dir is None:
        curr_src_file = os.path.join(src_root_dir, rel_src_file_name)
    else:
        curr_src_file = os.path.join(build_output_dir, rel_src_file_name)

    orig_build_args[-1] = curr_src_file
    modified_build_args = list()

    modified_build_args.append(clang_path)
    modified_build_args.append(EMIT_LLVM_FLAG)
    # Handle Target flags
    modified_build_args.append(ARCH_TARGET)
    if target_arch == ARM_32:
        modified_build_args.append(ARM_32_LLVM_ARCH)
    if target_arch == ARM_64:
        modified_build_args.append(ARM_64_LLVM_ARCH)
    # handle debug flags
    for curr_d_flg in DEBUG_INFO_FLAGS:
        modified_build_args.append(curr_d_flg)
    # handle optimization flags
    for curr_op in TARGET_OPTIMIZATION_FLAGS:
        modified_build_args.append(curr_op)

    for curr_war_op in DISABLE_WARNINGS:
        modified_build_args.append(curr_war_op)

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
    src_file_name = os.path.basename(curr_src_file)

    curr_output_dir = os.path.join(output_folder, src_dir_name)
    os.system('mkdir -p ' + curr_output_dir)

    curr_output_file = os.path.abspath(os.path.join(curr_output_dir, src_file_name[:-2] + '.llvm.bc'))
    output_file_idx = _get_output_file_idx(orig_build_args)
    assert (output_file_idx != -1)
    orig_build_args[output_file_idx] = curr_output_file

    for curr_op in orig_build_args:
        if _is_allowed_flag(curr_op):
            modified_build_args.append(curr_op)

    return ' '.join(modified_build_args)


def _generate_llvm_bitcode(kernel_src_dir, base_output_folder, makeout_file, gcc_prefix, target_arch, clang_path,
                           command_output_dir=None):
    output_llvm_sh_file = os.path.join(base_output_folder, 'llvm_build.sh')
    fp_out = open(output_llvm_sh_file, 'w')
    makeout_lines = open(makeout_file, 'r').readlines()
    llvm_cmds = []
    if command_output_dir is not None:
        llvm_cmds.append('cd ' + command_output_dir + '\n')
    for curr_line in makeout_lines:
        curr_line = curr_line.strip()
        if curr_line.startswith(gcc_prefix):
            llvm_mod_str = _get_llvm_build_str(kernel_src_dir, curr_line,
                                               base_output_folder, target_arch, clang_path,
                                               build_output_dir=command_output_dir)
            fp_out.write(llvm_mod_str + '\n')
            llvm_cmds.append(llvm_mod_str)
        elif len(curr_line.split()) > 2 and curr_line.split()[1] == gcc_prefix:
            llvm_mod_str = _get_llvm_build_str(kernel_src_dir, ' '.join(curr_line.split()[1:]),
                                               base_output_folder, target_arch, clang_path,
                                               build_output_dir=command_output_dir)
            fp_out.write(llvm_mod_str + '\n')
            llvm_cmds.append(llvm_mod_str)
    fp_out.close()
    log_info("Running LLVM Commands in multiprocessing mode.")
    build_src_dir = os.path.dirname(makeout_file)
    curr_dir = os.getcwd()
    os.chdir(build_src_dir)
    if command_output_dir is not None:
        os.chdir(command_output_dir)
    p = Pool(cpu_count())
    return_vals = p.map(_run_program, llvm_cmds)
    os.chdir(curr_dir)
    log_info("Finished Building LLVM Bitcode files")
    for i in range(len(return_vals)):
        if return_vals[i]:
            log_error("[-] Command Failed:" + llvm_cmds[i] + "\n")

    log_success("[+] Script containing all LLVM Build Commands:" + output_llvm_sh_file)
    return True
