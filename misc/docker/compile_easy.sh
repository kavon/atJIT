#!/bin/bash

build=$1
llvm_path=$2
easy_path=$3

rm -fr $easy_path/_build
mkdir -p $easy_path/_build
cd $easy_path/_build

flags="-DLLVM_DIR=$llvm_path"
case "$build" in
  ("debug") echo flags="$flags -DCMAKE_BUILD_TYPE=Debug" ;;
  ("dev") echo flags="$flags -DCMAKE_BUILD_TYPE=RelWithDebInfo" ;;
  ("release") echo flags="$flags -DCMAKE_BUILD_TYPE=Release" ;;
  (*) exit -1 ;;
esac

cmake .. $flags -G Ninja 
ninja 
