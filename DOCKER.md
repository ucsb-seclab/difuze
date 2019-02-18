# Running difuze from docker
______
The docker images `machiry/difuzecommon` and `machiry/difuze60` (for LLVM 6.0) contains the difuze sources (at `/difuze/repo`) along with all the dependencies installed.
## Install Docker
```
sudo apt-get install -y docker.io
```
## Running the docker image
```
docker run -i -t --privileged -v /dev/bus/usb:/dev/bus/usb machiry/difuzecommon /bin/bash
```
You need to give access to the `usb` devices to allow `adb` (from the docker container) to access the phone connected to the host machine.

## Using the docker image
### Building a kernel
1. Mount the kernel folder while running docker using `-v` command; do git pull and build all the sources.
```
docker run -i -t --privileged -v /dev/bus/usb:/dev/bus/usb machiry/difuzecommon -v <path_on_host_machine>:<path_in_docker> /bin/bash

// inside the docker container
// Install Bear
# cd /
# git clone https://github.com/Machiry/Bear.git
# cd Bear
# cmake .
# make install

// Get the latest difuze
# cd /difuze/repo
# git pull
# cd InterfaceHandlers
# ./build.sh

```
Example:
```
docker run -i -t --privileged -v /dev/bus/usb:/dev/bus/usb machiry/difuzecommon -v /home/difuze/kernels/msm:/androidkernels/msmkernel /bin/bash
```
2. Create `compile_commands.json`:

Configure the kernel with the desired config and while running make use following command:
```
$ cd /androidkernels/msmkernel
$ export CROSS_COMPILE=aarch64-linux-android-
$ bear make O=out ARCH=arm64 -j8
```

This will create `compile_commands.json`.

3. Running interface recovery:
Follow steps [1.3.2](https://github.com/ucsb-seclab/difuze#132-running-interface-recovery-analysis) (An example can be found at [1.4.2](https://github.com/ucsb-seclab/difuze#142-running-interface-recovery))
__Note: Make sure that you browse to `/difuze/repo` inside the docker image before running the above commands.__
4. Run the [post processing](https://github.com/ucsb-seclab/difuze#15-post-processing) and [the fuzzer](https://github.com/ucsb-seclab/difuze#2-fuzzing).
