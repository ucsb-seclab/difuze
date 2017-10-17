#!/usr/bin/env bash
BASEDIR=$(dirname "$0")
g++ $BASEDIR/src/main.cpp -o $BASEDIR/dr_linker `llvm-config --cxxflags --libs --ldflags --system-libs`
