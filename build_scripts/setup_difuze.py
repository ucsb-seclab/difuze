"""
This script clones and setups llvm and friends in the provided folder.
"""

import argparse
from multiprocessing import cpu_count
import os
import sys


def log_info(*args):
    log_str = "[*] "
    for curr_a in args:
        log_str = log_str + " " + str(curr_a)
    print log_str


def log_error(*args):
    log_str = "[!] "
    for curr_a in args:
        log_str = log_str + " " + str(curr_a)
    print log_str


def log_warning(*args):
    log_str = "[?] "
    for curr_a in args:
        log_str = log_str + " " + str(curr_a)
    print log_str


def log_success(*args):
    log_str = "[+] "
    for curr_a in args:
        log_str = log_str + " " + str(curr_a)
    print log_str
    

LLVM_GIT_HUB_BASE = "https://github.com/llvm-mirror/"
SPARSE_URL = "git://git.kernel.org/pub/scm/devel/sparse/sparse.git"


def setup_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('-b', action='store', dest='target_branch', default='release_38',
                        help='Branch (i.e. version) of the LLVM to setup. Default: release_38 e.g., release_38')

    parser.add_argument('-o', action='store', dest='output_folder',
                        help='Folder where everything needs to be setup.')

    return parser


def usage():
    log_error("Invalid Usage.")
    log_error("Run: python ", __file__, "--help", ", to know the correct usage.")
    sys.exit(-1)


def main():
    arg_parser = setup_args()
    parsed_args = arg_parser.parse_args()
    # step 1: Setup common dictionary
    reps_to_setup = {'tools': ['clang'], 'projects': ['compiler-rt', 'libcxx', 'libcxxabi', 'openmp']}
    if parsed_args.output_folder is None:
        usage()
    sparse_dir = os.path.join(parsed_args.output_folder, "sparse")
    base_output_dir = os.path.join(parsed_args.output_folder, "llvm")
    target_branch = parsed_args.target_branch
    log_info("Preparing setup in:", parsed_args.output_folder)
    log_info("Cloning sparse")
    backup_dir = os.getcwd()
    os.system('mkdir -p ' + sparse_dir)
    os.system("git clone " + SPARSE_URL + " " + sparse_dir)
    os.chdir(sparse_dir)
    os.system("make")
    os.chdir(backup_dir)
    log_success("Successfully built sparse.")
    log_info("Cloning LLVM")
    os.system('mkdir -p ' + str(base_output_dir))
    git_clone_cmd = "git clone " + LLVM_GIT_HUB_BASE + "llvm" + " -b " + \
                    str(target_branch) + " " + base_output_dir
    log_info("Setting up llvm.")
    os.system(git_clone_cmd)

    for curr_folder in reps_to_setup:
        rep_folder = os.path.join(base_output_dir, curr_folder)
        for curr_rep in reps_to_setup[curr_folder]:
            dst_folder = os.path.join(rep_folder, curr_rep)
            git_clone_cmd = "git clone " + LLVM_GIT_HUB_BASE + str(curr_rep) + " -b " + \
                            str(target_branch) + " " + dst_folder
            log_info("Setting up ", curr_rep, " repo.")
            os.system(git_clone_cmd)

    log_success("Cloned all the repositories.")
    build_dir = os.path.join(base_output_dir, "build")
    log_info("Trying to build in ", build_dir)
    os.system('mkdir -p ' + build_dir)
    os.chdir(build_dir)
    os.system('cmake ..')
    multi_proc_count = cpu_count() - 2
    if multi_proc_count > 0:
        log_info("Building in multiprocessing mode on ", multi_proc_count, " cores.")
        os.system('make -j' + str(multi_proc_count))
    else:
        log_info("Building in single core mode.")
        os.system('make')
    log_success("Build Complete.")
    print ""
    log_success("Add following lines to your .bashrc")
    print("export LLVM_ROOT=", build_dir)
    print("export PATH=$LLVM_ROOT/bin:" + sparse_dir + ":$PATH")
    print ""
    log_success("After adding the above lines to .bashrc.\nPlease run: source ~/.bashrc on the "
                "terminal for the changes to take effect.")
    print ""
    log_success("Setup Complete.")
    os.chdir(backup_dir)


if __name__ == "__main__":
    main()
