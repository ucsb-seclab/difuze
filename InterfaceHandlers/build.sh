#!/usr/bin/env bash
BASEDIR=$(dirname "$0")
echo "[*] Trying to Build AnalysisHelpers"
cd $BASEDIR/AnalysisHelpers
./build.sh
cd ..
echo "[*] Trying to Build MainAnalysisPasses"
cd $BASEDIR/MainAnalysisPasses
./build.sh
