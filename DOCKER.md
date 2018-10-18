# Running difuze from docker
______
The docker image `machiry/difuzebase` contains the difuze sources (at `/difuze`) along with all the dependencies installed.
## Install Docker
```
sudo apt-get install -y docker.io
```
## Running the docker image
```
docker run -i -t --privileged -v /dev/bus/usb:/dev/bus/usb machiry/difuzebase /bin/bash
```
You need to give access to the `usb` devices to allow `adb` (from the docker container) to access the phone connected to the host machine.

## Using the docker image
### Building a kernel
1. Mount the kernel folder while running docker using `-v` command.
```
docker run -i -t --privileged -v /dev/bus/usb:/dev/bus/usb machiry/difuzesetup -v <path_on_host_machine>:<path_in_docker> /bin/bash
```
Example:
```
docker run -i -t --privileged -v /dev/bus/usb:/dev/bus/usb machiry/difuzesetup -v /home/difuze/kernels/msm:/androidkernels/msmkernel /bin/bash
```
2. Create `makeout.txt`
Configure the kernel with the desired config and while running make use following command:
```
make V=1 O=out ARCH=arm64 > makeout.txt 2>&1
```
This will create `makeout.txt`.
3. Running interface recovery:
Follow steps [1.3.2](https://github.com/ucsb-seclab/difuze#132-running-interface-recovery-analysis) (An example can be found at [1.4.2](https://github.com/ucsb-seclab/difuze#142-running-interface-recovery))

4. Run the [post processing](https://github.com/ucsb-seclab/difuze#15-post-processing) and [the fuzzer](https://github.com/ucsb-seclab/difuze#2-fuzzing).
