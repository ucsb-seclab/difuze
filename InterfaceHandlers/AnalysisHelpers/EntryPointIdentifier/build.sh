#!/usr/bin/env bash
BASEDIR=$(dirname "$0")
g++ $BASEDIR/src/main.cpp -fpermissive -o $BASEDIR/entry_point_handler `llvm-config --cxxflags --libs --ldflags --system-libs`