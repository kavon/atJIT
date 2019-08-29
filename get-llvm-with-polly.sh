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
git reset --hard bb7d03ce5b3ce38e4de3b784961428957d73ea03

cd ./tools

## PULL CLANG
git clone --branch pragma --single-branch https://github.com/Meinersbur/clang.git
cd ./clang
git reset --hard 001a843fb3bc7ab567eb605322df07471c1a4583
cd ..

# PULL POLLY
git clone --branch pragma --single-branch https://github.com/Meinersbur/polly.git
cd ./polly
git reset --hard ca3e663094d773988a3916b8f1b2585bd351240b
cd ..

cd ../..

##################
## configure & build

mkdir build install
cd ./build
cmake -C $DIR/cmake/LLVM.cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=../install ../src

$BUILD_CMD
