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
git reset --hard 87877c50435bba433b7fa261574b7e3c4372ea5a

cd ./tools

## PULL CLANG
git clone --branch pragma --single-branch https://github.com/Meinersbur/clang.git
cd ./clang
git reset --hard 2c565d6149be499a22ae3089eae37b0a07057b15
cd ..

# PULL POLLY
git clone --branch pragma --single-branch https://github.com/Meinersbur/polly.git
cd ./polly
git reset --hard 0abadf4cea38d9f6cf2236588e551ebfdcb80590
cd ..

cd ../..

##################
## configure & build

mkdir build install
cd ./build
cmake -C $DIR/cmake/LLVM.cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=../install ../src

$BUILD_CMD
