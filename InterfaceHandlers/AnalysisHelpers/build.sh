#!/usr/bin/env bash
BASEDIR=$(dirname "$0")
echo "[*] Trying to Build Dr_linker"
cd $BASEDIR/Dr_linker
./build.sh
cd ..
echo "[*] Trying to Build EntryPointIdentifier"
cd $BASEDIR/EntryPointIdentifier
./build.sh
