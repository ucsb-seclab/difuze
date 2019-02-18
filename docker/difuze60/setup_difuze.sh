#!/bin/bash
mkdir /difuze
cd /difuze
git clone https://github.com/Machiry/Bear.git
cd Bear
cmake .
make install
cd ..
export LLVM_ROOT=/dr_checker/drcheckerdeps/llvm/build
export PATH=$LLVM_ROOT/bin:/dr_checker/drcheckerdeps/sparse:$PATH
git clone https://github.com/ucsb-seclab/difuze.git -b llvm_6.0 repo
cd repo
cd InterfaceHandlers
./build.sh
echo "export PATH=/dr_checker/drcheckerdeps/aarch64toolchain/bin:\$PATH" >> ~/.bashrc
