#!/bin/bash

####
# This script obtains and builds a version of LLVM that has
# special extensions for supporting polly transformation
# annotations in the LLVM IR.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

LLVM_REV=64bb270506ea371f015a7da370518a9512dc10ba

if [ $# -ne 1 ]; then
  echo "expected args:  <path to empty dir>"
  exit 1
fi

NUM_CPUS=`getconf _NPROCESSORS_ONLN`

GENERATOR="Unix Makefiles"
BUILD_CMD="make install -j${NUM_CPUS}"

pushd $1

#####
# make sure we're in an empty dir
if [ `ls -1A . | wc -l` -ne 0 ]; then
  echo "provided directory must be empty!"
  exit 1
fi

#####
# get sources

git clone --no-checkout https://github.com/llvm-mirror/llvm.git src
pushd src
git checkout $LLVM_REV

pushd tools

git clone --branch pragma --single-branch --depth 1 https://github.com/Meinersbur/clang.git

git clone --branch pragma --single-branch --depth 1 https://github.com/Meinersbur/polly.git

popd
popd

##################
## configure & build

mkdir build install
pushd build
cmake -C $DIR/cmake/LLVM.cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=../install ../src

$BUILD_CMD
