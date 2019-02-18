FROM machiry/llvm_60
RUN apt-get -y update
ADD setup_difuze.sh /tmp/setup_difuze.sh
ADD android_arm64_prebuilt /dr_checker/drcheckerdeps/aarch64toolchain
RUN chmod a+rx /tmp/setup_difuze.sh
RUN /bin/bash /tmp/setup_difuze.sh
CMD ["tail", "-f", "/dev/null" ]


