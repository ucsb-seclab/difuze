difuze: Fuzzer for Linux Kernel Drivers
===================

[![License](https://img.shields.io/github/license/angr/angr.svg)](https://github.com/ucsb-seclab/difuze/blob/master/LICENSE)

This repo contains all the sources (including setup scripts), you need to get `difuze` up and running.
### Tested on
Ubuntu >= 14.04.5 LTS

As explained in our paper, There are two main components of `difuze`: **Interface Recovery** and **Fuzzing Engine**

## 1. Interface Recovery
The Interface recovery mechanism is based on LLVM analysis passes. Every step of interface recovery are written as  individual passes. Follow the below instructions on how to get the *Interface Recovery* up and running.

### 1.1 Setup
This step takes care of installing LLVM and `c2xml`:

First, make sure that you have libxml (required for c2xml):
```
sudo apt-get install libxml2-dev
```

Next, We have created a single script, which downloads and builds all the required tools.
```
cd build_scripts
python setup_difuze.py --help
usage: setup_difuze.py [-h] [-b TARGET_BRANCH] [-o OUTPUT_FOLDER]

optional arguments:
  -h, --help        show this help message and exit
  -b TARGET_BRANCH  Branch (i.e. version) of the LLVM to setup. Default:
                    release_38 e.g., release_38
  -o OUTPUT_FOLDER  Folder where everything needs to be setup.

```
Example:
```
python setup_difuze.py -o difuze_deps
```
To complete the setup you also need modifications to your local `PATH` environment variable. The setup script will give you exact changes you need to do.
### 1.2 Building
This depends on the successful completion of [Setup](#11-setup).
We have a single script that builds everything, you are welcome.
```
cd InterfaceHandlers
./build.sh
```
### 1.3 Running
This depends on the successful completion of [Build](#12-building).
To run the Interface Recovery components on kernel drivers, we need to first the drivers into llvm bitcode.
#### 1.3.1 Building kernel
First, we need to have a buildable kernel. Which means you should be able to compile the kernel using regular build setup. i.e., `make`.
We first capture the output of `make` command, from this output we extract the exact compilation command.
##### 1.3.1.1 Generating output of `make` (or `makeout.txt`)

Just pass `V=1` and redirect the output to the file.
Example:
```
make V=1 O=out ARCH=arm64 > makeout.txt 2>&1
```
**NOTE: DO NOT USE MULTIPLE PROCESSES** i.e., `-j`. Running in multi-processing mode will mess up the output file as multiple process try to write to the output file.

That's it. Next, in the following step our script takes the generated `makeout.txt` and run the Interface Recovery on all the recognized drivers.
#### 1.3.2 Running Interface Recovery analysis
All the various steps of Interface Recovery are wrapped in a single script `build_scripts/run_all.py`
How to run:
```
cd build_scripts
python run_all.py --help

usage: run_all.py [-h] [-l LLVM_BC_OUT] [-a CHIPSET_NUM] [-m MAKEOUT]
                  [-g COMPILER_NAME] [-n ARCH_NUM] [-o OUT]
                  [-k KERNEL_SRC_DIR] [-skb] [-skl] [-skp] [-skP] [-ske]
                  [-skI] [-ski] [-skv] [-skd] [-f IOCTL_FINDER_OUT]

optional arguments:
  -h, --help           show this help message and exit
  -l LLVM_BC_OUT       Destination directory where all the generated bitcode
                       files should be stored.
  -a CHIPSET_NUM       Chipset number. Valid chipset numbers are:
                       1(mediatek)|2(qualcomm)|3(huawei)|4(samsung)
  -m MAKEOUT           Path to the makeout.txt file.
  -g COMPILER_NAME     Name of the compiler used in the makeout.txt, This is
                       needed to filter out compilation commands. Ex: aarch64
                       -linux-android-gcc
  -n ARCH_NUM          Destination architecture, 32 bit (1) or 64 bit (2).
  -o OUT               Path to the out folder. This is the folder, which could
                       be used as output directory during compiling some
                       kernels.
  -k KERNEL_SRC_DIR    Base directory of the kernel sources.
  -skb                 Skip LLVM Build (default: not skipped).
  -skl                 Skip Dr Linker (default: not skipped).
  -skp                 Skip Parsing Headers (default: not skipped).
  -skP                 Skip Generating Preprocessed files (default: not
                       skipped).
  -ske                 Skip Entry point identification (default: not skipped).
  -skI                 Skip Generate Includes (default: not skipped).
  -ski                 Skip IoctlCmdParser run (default: not skipped).
  -skv                 Skip V4L2 ioctl processing (default: not skipped).
  -skd                 Skip Device name finder (default: not skipped).
  -f IOCTL_FINDER_OUT  Path to the output folder where the ioctl command
                       finder output should be stored.
```
The script builds, links and runs Interface Recovery on all the recognized drivers, as such it might take **considerable time(45 min-90 min)**. 

The above script performs following tasks in a multiprocessor mode to make use of all CPU cores:
##### 1.3.2.1 LLVM Build 
* Enabled by default.

All the bitcode files generated will be placed in the folder provided to the argument `-l`.
This step takes considerable time, depending on the number of cores you have. 
So, if you had already done this step, You can skip this step by passing `-skb`. 
##### 1.3.2.2 Linking all driver bitcode files in s consolidated bitcode file.
* Enabled by default

This performs linking, it goes through all the bitcode files and identifies the related bitcode files that need to be linked and links them (using `llvm-link`) in to a consolidated bitcode file (which will be stored along side corresponding bitcode file).

Similar to the above step, you can skip this step by passing `-skl`.
##### 1.3.2.3 Parsing headers to identify entry function fields.
* Enabled by default.

This step looks for the entry point declarations in the header files and stores their configuration in the file: `hdr_file_config.txt` under LLVM build directory.

To skip: `-skp`
##### 1.3.2.4 Identify entry points in all the consolidated bitcode files.
* Enabled by default

This step identifies all the entry points across all the driver consolidated bitcode files.
The output will be stored in file: `entry_point_out.txt` under LLVM build directory.

Example of contents in the file `entry_point_out.txt`:
```
IOCTL:msm_lsm_ioctl:/home/difuze/kernels/pixel/msm/sound/soc/msm/qdsp6v2/msm-lsm-client.c:msm_lsm_ioctl.txt:/home/difuze/pixel/llvm_out/sound/soc/msm/qdsp6v2/llvm_link_final/final_to_check.bc
IOCTL:msm_pcm_ioctl:/home/difuze/kernels/pixel/msm/sound/soc/msm/qdsp6v2/msm-pcm-lpa-v2.c:msm_pcm_ioctl.txt:/home/difuze/pixel/llvm_out/sound/soc/msm/qdsp6v2/llvm_link_final/final_to_check.bc

```
To skip: `-ske`
##### 1.3.2.5 Run Ioctl Cmd Finder on all the identified entry points.
* Enabled by default.

This step will run the main Interface Recovery component (`IoctlCmdParser`) on all the entry points in the file `entry_point_out.txt`. The output for each entry point will be stored in the folder provided for option `-f`.

To skip: `-ski`
