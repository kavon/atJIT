#!/bin/bash

dir=$2
cd $dir

wget -O ./llvm.zip https://github.com/llvm-mirror/llvm/archive/release_50.zip
wget -O ./clang.zip https://github.com/llvm-mirror/clang/archive/release_50.zip

unzip llvm.zip && rm -f llvm.zip && mv llvm-release_50 llvm
unzip clang.zip && rm -f clang.zip && mv clang-release_50 llvm/tools/clang

mkdir -p llvm/_build
cd llvm/_build

build=$1
asserts_and_ccache="-DLLVM_ENABLE_ASSERTIONS=1 -DLLVM_CCACHE_BUILD=1"
flags="-DLLVM_ENABLE_RTTI=1 -DLLVM_TARGETS_TO_BUILD=Native -DBUILD_SHARED_LIBS=1 -DCMAKE_INSTALL_PREFIX=`pwd`/../_install -DLLVM_INSTALL_UTILS=1"
case "$build" in
  ("debug") echo flags="$flags -DCMAKE_BUILD_TYPE=Debug $asserts_and_ccache" ;;
  ("dev") echo flags="$flags -DCMAKE_BUILD_TYPE=RelWithDebInfo $asserts_and_ccache" ;;
  ("release") echo flags="$flags -DCMAKE_BUILD_TYPE=Release" ;;
  (*) exit -1 ;;
esac

cmake .. $flags -G Ninja 
ninja 
ninja install
