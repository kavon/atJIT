#!/bin/bash

####
# This script obtains and builds a version of LLVM that has
# special extensions for supporting polly transformation
# annotations in the LLVM IR.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

if [ $# -ne 1 ]; then
  echo "expected args:  <path to empty dir>"
  exit 1
fi

############
# prefer using Ninja if available
command -v ninja
if [ $? -eq 0 ]; then
  GENERATOR="Ninja"
  BUILD_CMD="ninja install"
else
  NUM_CPUS=`getconf _NPROCESSORS_ONLN`
  GENERATOR="Unix Makefiles"
  BUILD_CMD="make install -j${NUM_CPUS}"
fi


cd $1

#####
# make sure we're in an empty dir
if [ `ls -1A . | wc -l` -ne 0 ]; then
  echo "provided directory must be empty!"
  exit 1
fi

#####
# get sources

# PULL LLVM
git clone --branch pragma --single-branch https://github.com/Meinersbur/llvm.git src
cd ./src
git reset --hard cf50706d0b788f21f58e0ac7f9fef8c3f010a6f1

cd ./tools

## PULL CLANG
git clone --branch pragma --single-branch https://github.com/Meinersbur/clang.git
cd ./clang
git reset --hard d0dc69f8de9250e68233ee8b56db5b2517d4aaf3
cd ..

# PULL POLLY
git clone --branch pragma --single-branch https://github.com/Meinersbur/polly.git
cd ./polly
git reset --hard 70baac1f268f77e976f9b0805db7ff5c14c06df2
cd ..

cd ../..

##################
## configure & build

mkdir build install
cd ./build
cmake -C $DIR/cmake/LLVM.cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=../install ../src

$BUILD_CMD
