from base_component import *
import os
from multiprocessing import Pool, cpu_count
from bear_helper import parse_compile_json
from bear_build_helper import *


class BearLLVMBuild(Component):
    """
        Component which builds LLVM Bitcode files.
    """
    def __init__(self, value_dict):
        clang_bin = None
        llvm_link_bin = None
        llvm_bc_out = None
        arch_num = None
        separate_out = None
        compile_json = None
        kernel_src_dir = ""
        is_clang_build = False
        if 'clangbin' in value_dict:
            clang_bin = value_dict['clangbin']
        if 'llvm_bc_out' in value_dict:
            llvm_bc_out = value_dict['llvm_bc_out']
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
        if 'is_clang_build' in value_dict:
            is_clang_build = value_dict['is_clang_build']

        self.clang_bin = clang_bin
        self.llvm_bc_out = llvm_bc_out
        self.arch_num = arch_num
        self.separate_out = separate_out
        self.kernel_src_dir = kernel_src_dir
        self.compile_json = compile_json
        self.llvm_link_bin = llvm_link_bin
        self.is_clang_build = is_clang_build

    def setup(self):
        if (not os.path.exists(self.clang_bin)) or (not os.path.exists(self.llvm_link_bin)):
            return "Required files (" + str(self.clang_bin) + ", " + \
                   str(self.llvm_link_bin) + ") do not exist"
        if self.llvm_bc_out is not None:
            if not os.path.exists(self.llvm_bc_out):
                os.makedirs(self.llvm_bc_out)
            if os.path.isfile(self.llvm_bc_out):
                return "Provided LLVM Bitcode out:" + self.llvm_bc_out + " is a file, but should be a directory path."
        else:
            return "LLVM Bitcode out should be provided."
        if self.arch_num is None:
            return "No architecture number provided."
        if self.compile_json is None or not os.path.exists(self.compile_json):
            return "No compile json or the provided file:" + str(self.compile_json) + " doesn't exist."
        return None

    def perform(self):
        comp_cmds, link_cmds = parse_compile_json(self.compile_json)
        return build_drivers(comp_cmds, link_cmds, self.kernel_src_dir, self.arch_num, self.clang_bin,
                             self.llvm_link_bin, self.llvm_bc_out, self.is_clang_build)

    def get_name(self):
        return "BearLLVMBuild"

    def is_critical(self):
        # Yes, this component is critical.
        return True


def _get_llvm_build_str(clang_path, build_args, src_root_dir, target_arch, work_dir,
                        src_file_path, output_file_path, llvm_bit_code_out):
    """
        Given a compilation command from the json, this function returns the clang based build string.
        assuming that the original was built with gcc
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

    curr_output_file = os.path.abspath(os.path.join(curr_output_dir, src_file_name[:-2] + '.llvm.bc'))

    for curr_op in build_args:
        if is_gcc_flag_allowed(curr_op):
            modified_build_args.append(curr_op)

    # tell clang to compile.
    modified_build_args.append("-c")
    modified_build_args.append(curr_src_file)
    modified_build_args.append("-o")
    modified_build_args.append(curr_output_file)

    return work_dir, output_file_path, curr_output_file, ' '.join(modified_build_args)


def _get_llvm_build_str_from_llvm(clang_path, build_args,
                                  src_root_dir, target_arch, work_dir,
                                  src_file_path, output_file_path,
                                  llvm_bit_code_out):
    """
        Given a compilation command from the json, this function returns the clang based build string.
        assuming that the original build was done with clang.
    :param clang_path: Path to clang.
    :param build_args: original arguments to the compiler.
    :param src_root_dir: Path to the kernel source directory.
    :param target_arch: Number representing target architecture.
    :param work_dir: Directory where the original command was run.
    :param src_file_path: Path to the source file being compiled.
    :param output_file_path: Path to the original object file.
    :param llvm_bit_code_out: Folder where all the linked bitcode files should be stored.
    :return:
    """

    curr_src_file = src_file_path
    modified_build_args = list()

    modified_build_args.append(clang_path)
    modified_build_args.append(EMIT_LLVM_FLAG)
    # handle debug flags
    for curr_d_flg in DEBUG_INFO_FLAGS:
        modified_build_args.append(curr_d_flg)
    # handle optimization flags
    for curr_op in TARGET_OPTIMIZATION_FLAGS:
        modified_build_args.append(curr_op)

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

    curr_output_file = os.path.abspath(os.path.join(curr_output_dir, src_file_name[:-2] + '.llvm.bc'))

    for curr_op in build_args:
        # ignore only optimization flags.
        if str(curr_op)[:2] != "-O":
            modified_build_args.append(curr_op)

    # tell clang to compile.
    modified_build_args.append("-c")
    modified_build_args.append(curr_src_file)
    modified_build_args.append("-o")
    modified_build_args.append(curr_output_file)

    return work_dir, output_file_path, curr_output_file, ' '.join(modified_build_args)


def _get_llvm_link_str(llvm_link_path, src_root_dir, input_files, input_bc_map,
                       output_file, work_dir, llvm_bit_code_out):
    """
        Given a linker command from the json, this function converts it into corresponding
        llvm-link command with all the correct parameters.
    :param llvm_link_path: Path to llvm-link
    :param src_root_dir: Path to the kernel source directory.
    :param input_files: input files for the linker.
    :param input_bc_map: Map containing object files to corresponding bitcode file.
    :param output_file: Original output object file path.
    :param work_dir: Directory where the original command was run.
    :param llvm_bit_code_out: Folder where all the linked bitcode files should be stored.
    :return:
    """
    modified_build_args = list()
    modified_build_args.append(llvm_link_path)
    for curr_input_file in input_files:
        if curr_input_file not in input_bc_map:
            return None
        target_bc_file = input_bc_map[curr_input_file]
        if not os.path.exists(target_bc_file):
            return None
        else:
            modified_build_args.append(input_bc_map[curr_input_file])

    rel_output_file = output_file
    if str(output_file).startswith("../"):
        rel_output_file = output_file[3:]
    if str(output_file).startswith('/'):
        rel_output_file = os.path.abspath(output_file)
        if src_root_dir[-1] == '/':
            rel_output_file = rel_output_file[len(src_root_dir):]
        else:
            rel_output_file = rel_output_file[len(src_root_dir) + 1:]
    # replace output file with llvm bc file
    out_dir_name = os.path.dirname(rel_output_file)
    output_file_name = os.path.basename(output_file)

    curr_output_dir = os.path.join(llvm_bit_code_out, out_dir_name)
    os.system('mkdir -p ' + curr_output_dir)

    curr_output_file = os.path.abspath(os.path.join(curr_output_dir, output_file_name[:-2] + '.final.linked.bc'))
    # append output file path
    modified_build_args.append("-o")
    modified_build_args.append(curr_output_file)
    return work_dir, output_file, curr_output_file, ' '.join(modified_build_args)


def _process_recursive_linker_commands(linker_commands, kernel_src_dir, llvm_link_path, llvm_bit_code_out,
                                       obj_bc_map, output_fp):
    """
        Function that handles recursive linker commands.
    :param linker_commands: Linker commands that needs to be recursively resolved.
    :param kernel_src_dir: Path to the kernel source directory.
    :param llvm_link_path: Path to llvm-link
    :param llvm_bit_code_out: Folder where all the linked bitcode files should be stored.
    :param obj_bc_map: Map containing object files to corresponding bitcode file.
    :param output_fp: Path where the linker commands should be stored.
    :return: None
    """
    recursive_linker_commands = []
    all_linker_commands = []
    for curr_linked_command in linker_commands:
        curr_ret_val = _get_llvm_link_str(llvm_link_path, kernel_src_dir,
                                          curr_linked_command.input_files, obj_bc_map,
                                          curr_linked_command.output_file,
                                          curr_linked_command.work_dir, llvm_bit_code_out)
        if curr_ret_val is not None:
            wd, obj_file, bc_file, build_str = curr_ret_val
            all_linker_commands.append((wd, build_str))
            obj_bc_map[obj_file] = bc_file
            output_fp.write("cd " + wd + ";" + build_str + "\n")
        else:
            recursive_linker_commands.append(curr_linked_command)

    if len(all_linker_commands) > 0:
        log_info("Got", len(all_linker_commands), "recursively resolved linker commands.")
        log_info("Running linker commands in multiprocessing mode.")
        p = Pool(cpu_count())
        return_vals = p.map(run_program_with_wd, all_linker_commands)
        log_success("Finished running linker commands.")

        if len(recursive_linker_commands) > 0:
            _process_recursive_linker_commands(recursive_linker_commands, kernel_src_dir, llvm_link_path,
                                               llvm_bit_code_out, obj_bc_map, output_fp)
    else:
        # we didn't resolve any new objects..but there are still some recursive commands.
        # we cannot resolve them.
        # bail out.
        if len(recursive_linker_commands) > 0:
            log_error("Failed to link following driver objects.")
            for curr_com in recursive_linker_commands:
                log_error(curr_com.output_file)


def build_drivers(compilation_commands, linker_commands, kernel_src_dir,
                  target_arch, clang_path, llvm_link_path, llvm_bit_code_out, is_clang_build):
    """
        The main method that performs the building and linking of the driver files.
    :param compilation_commands: Parsed compilation commands from the json.
    :param linker_commands: Parsed linker commands from the json.
    :param kernel_src_dir: Path to the kernel source directory.
    :param target_arch: Number representing target architecture.
    :param clang_path: Path to clang.
    :param llvm_link_path: Path to llvm-link
    :param llvm_bit_code_out: Folder where all the linked bitcode files should be stored.
    :param is_clang_build: Flag to indicate that this is a clang build.
    :return: True
    """
    output_llvm_sh_file = os.path.join(llvm_bit_code_out, 'llvm_build.sh')
    fp_out = open(output_llvm_sh_file, 'w')
    fp_out.write("#!/bin/bash\n")
    log_info("Writing all compilation commands to", output_llvm_sh_file)
    all_compilation_commands = []
    obj_bc_map = {}
    for curr_compilation_command in compilation_commands:
        if is_clang_build:
            wd, obj_file, bc_file, build_str = _get_llvm_build_str_from_llvm(clang_path, curr_compilation_command.curr_args,
                                                                             kernel_src_dir, target_arch,
                                                                             curr_compilation_command.work_dir,
                                                                             curr_compilation_command.src_file,
                                                                             curr_compilation_command.output_file,
                                                                             llvm_bit_code_out)
        else:
            wd, obj_file, bc_file, build_str = _get_llvm_build_str(clang_path, curr_compilation_command.curr_args,
                                                                   kernel_src_dir, target_arch,
                                                                   curr_compilation_command.work_dir,
                                                                   curr_compilation_command.src_file,
                                                                   curr_compilation_command.output_file, llvm_bit_code_out)
        all_compilation_commands.append((wd, build_str))
        obj_bc_map[obj_file] = bc_file
        fp_out.write("cd " + wd + ";" + build_str + "\n")
    fp_out.close()

    log_info("Got", len(all_compilation_commands), "compilation commands.")
    log_info("Running compilation commands in multiprocessing modea.")
    p = Pool(cpu_count())
    return_vals = p.map(run_program_with_wd, all_compilation_commands)
    log_success("Finished running compilation commands.")

    output_llvm_sh_file = os.path.join(llvm_bit_code_out, 'llvm_link_cmds.sh')
    fp_out = open(output_llvm_sh_file, 'w')
    fp_out.write("#!/bin/bash\n")
    log_info("Writing all linker commands to", output_llvm_sh_file)
    all_linker_commands = []
    recursive_linker_commands = []
    for curr_linked_command in linker_commands:
        curr_ret_val = _get_llvm_link_str(llvm_link_path, kernel_src_dir,
                                          curr_linked_command.input_files, obj_bc_map,
                                          curr_linked_command.output_file,
                                          curr_linked_command.work_dir, llvm_bit_code_out)
        if curr_ret_val is not None:
            wd, obj_file, bc_file, build_str = curr_ret_val
            all_linker_commands.append((wd, build_str))
            obj_bc_map[obj_file] = bc_file
            fp_out.write("cd " + wd + ";" + build_str + "\n")
        else:
            # these are recursive linker commands.
            recursive_linker_commands.append(curr_linked_command)

    log_info("Got", len(all_linker_commands), "regular linker commands.")
    log_info("Running linker commands in multiprocessing mode.")
    p = Pool(cpu_count())
    return_vals = p.map(run_program_with_wd, all_linker_commands)
    log_success("Finished running linker commands.")

    if len(recursive_linker_commands) > 0:
        log_info("Got", len(recursive_linker_commands), " recursive linker commands.")
        _process_recursive_linker_commands(recursive_linker_commands, kernel_src_dir, llvm_link_path,
                                           llvm_bit_code_out, obj_bc_map, fp_out)
    fp_out.close()

    return True
