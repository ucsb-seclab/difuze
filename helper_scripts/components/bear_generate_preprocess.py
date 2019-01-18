from base_component import *
import os
from multiprocessing import Pool, cpu_count
from bear_helper import parse_compile_json


class BearGeneratePreprocessed(Component):
    """
        Component which generates preprocessed files from
        compilation command.
    """
    def __init__(self, value_dict):
        makeout_file = None
        clang_bin = None
        llvm_link_bin = None
        llvm_bc_out = None
        compiler_name = None
        arch_num = None
        separate_out = None
        compile_json = None
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
        if 'compile_json' in value_dict:
            compile_json = value_dict["compile_json"]
        if 'llvmlinkbin' in value_dict:
            llvm_link_bin = value_dict["llvmlinkbin"]

        self.curr_makeout = makeout_file
        self.clang_bin = clang_bin
        self.llvm_bc_out = llvm_bc_out
        self.gcc_name = compiler_name
        self.arch_num = arch_num
        self.separate_out = separate_out
        self.kernel_src_dir = kernel_src_dir
        self.compile_json = compile_json
        self.llvm_link_bin = llvm_link_bin

    def setup(self):
        if (not os.path.exists(self.curr_makeout)) or \
                (not os.path.exists(self.clang_bin)) or (not os.path.exists(self.llvm_link_bin)):
            return "Required files(" + str(self.curr_makeout) + ", " + str(self.clang_bin) + \
                   ", " + str(self.llvm_link_bin)  + ") do not exist"
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
        if self.compile_json is None or not os.path.exists(self.compile_json):
            return "No compile json or the provided file:" + str(self.compile_json) + " doesn't exist."
        return None

    def perform(self):
        comp_cmds, link_cmds = parse_compile_json(self.compile_json)
        return build_preprocessed(comp_cmds, link_cmds, self.kernel_src_dir, self.arch_num, self.clang_bin,
                                  self.llvm_link_bin, self.llvm_bc_out)

    def get_name(self):
        return "BearGeneratePreprocessed"

    def is_critical(self):
        # Yes, this component is critical.
        return False

# UTILITIES FUNCTION
# These flags should be removed from gcc cmdline
INVALID_GCC_FLAGS = ['-mno-thumb-interwork', '-fconserve-stack', '-fno-var-tracking-assignments',
                     '-fno-delete-null-pointer-checks', '--param=allow-store-data-races=0',
                     '-Wno-unused-but-set-variable', '-Werror=frame-larger-than=1', '-Werror', '-Wall',
                     '-fno-jump-tables', '-nostdinc', '-mpc-relative-literal-loads', '-mabi=lp64']
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


def _run_program((workdir, cmd_to_run)):
    """
        Run the given program with in the provided directory.
    :return: None
    """
    curr_dir = os.getcwd()
    os.chdir(workdir)
    os.system(cmd_to_run)
    os.chdir(curr_dir)


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


def _get_llvm_preprocessing_str(clang_path, build_args, src_root_dir, target_arch, work_dir,
                                src_file_path, output_file_path, llvm_bit_code_out):
    """
        Given a compilation command from the json, this function returns the clang
        based preprocessing string.
    :param clang_path: Path to clang.
    :param build_args: original arguments to the compiler.
    :param src_root_dir: Path to the kernel source directory.
    :param target_arch: Number representing target architecture/
    :param work_dir: Directory where the original command was run.
    :param src_file_path: Path to the source file being compiled.
    :param output_file_path: Path to the original object file.
    :param llvm_bit_code_out: Folder where all the linked bitcode files should be stored.
    :return:
    """

    curr_src_file = src_file_path
    modified_build_args = list()

    modified_build_args.append(clang_path)
    modified_build_args.append('-E')
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

    rel_src_file_name = curr_src_file
    if str(curr_src_file).startswith("../"):
        rel_src_file_name = curr_src_file[3:]
    if str(curr_src_file).startswith('/'):
        rel_src_file_name = os.path.abspath(curr_src_file)
        if src_root_dir[-1] == '/':
            rel_src_file_name = rel_src_file_name[len(src_root_dir):]
        else:
            rel_src_file_name = rel_src_file_name[len(src_root_dir) + 1:]
    # replace output file with llvm bc file
    src_dir_name = os.path.dirname(rel_src_file_name)
    src_file_name = os.path.basename(curr_src_file)

    curr_output_dir = os.path.join(llvm_bit_code_out, src_dir_name)
    os.system('mkdir -p ' + curr_output_dir)

    curr_output_file = os.path.abspath(os.path.join(curr_output_dir, src_file_name[:-2] + '.preprocessed'))

    for curr_op in build_args:
        if _is_allowed_flag(curr_op):
            modified_build_args.append(curr_op)

    # tell clang to compile.
    modified_build_args.append("-c")
    modified_build_args.append(curr_src_file)
    modified_build_args.append("-o")
    modified_build_args.append(curr_output_file)

    return work_dir, output_file_path, curr_output_file, ' '.join(modified_build_args)


def build_preprocessed(compilation_commands, linker_commands, kernel_src_dir,
                       target_arch, clang_path, llvm_link_path, llvm_bit_code_out):
    """
        The main method that performs the preprocessing.
    :param compilation_commands: Parsed compilation commands from the json.
    :param linker_commands: Parsed linker commands from the json.
    :param kernel_src_dir: Path to the kernel source directory.
    :param target_arch: Number representing target architecture.
    :param clang_path: Path to clang.
    :param llvm_link_path: Path to llvm-link
    :param llvm_bit_code_out: Folder where all the linked bitcode files should be stored.
    :return: True
    """
    output_llvm_sh_file = os.path.join(llvm_bit_code_out, 'llvm_generate_preprocessed.sh')
    fp_out = open(output_llvm_sh_file, 'w')
    fp_out.write("#!/bin/bash\n")
    log_info("Writing all preprocessing commands to", output_llvm_sh_file)
    all_compilation_commands = []
    for curr_compilation_command in compilation_commands:
        wd, obj_file, bc_file, build_str = _get_llvm_preprocessing_str(clang_path, curr_compilation_command.curr_args,
                                                                       kernel_src_dir, target_arch,
                                                                       curr_compilation_command.work_dir,
                                                                       curr_compilation_command.src_file,
                                                                       curr_compilation_command.output_file,
                                                                       llvm_bit_code_out)
        all_compilation_commands.append((wd, build_str))
        fp_out.write("cd " + wd + ";" + build_str + "\n")
    fp_out.close()

    log_info("Got", len(all_compilation_commands), "preprocessing commands.")
    log_info("Running preprocessing commands in multiprocessing modea.")
    p = Pool(cpu_count())
    return_vals = p.map(_run_program, all_compilation_commands)
    log_success("Finished running preprocessing commands.")

    return True
