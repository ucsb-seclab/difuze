import os

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
EMIT_LLVM_FLAG = '-emit-llvm'


def run_program_with_wd((workdir, cmd_to_run)):
    """
        Run the given program with in the provided directory.
    :return: None
    """
    curr_dir = os.getcwd()
    os.chdir(workdir)
    os.system(cmd_to_run)
    os.chdir(curr_dir)


def is_gcc_flag_allowed(curr_flag):
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
