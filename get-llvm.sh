#!/bin/bash

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

# PULL LLVM mono repo
git clone https://github.com/llvm/llvm-project.git src
cd ./src
git checkout tags/llvmorg-8.0.1

cd ..

##################
## configure & build

mkdir build install
cd ./build
cmake -C $DIR/cmake/LLVM.cmake -G "$GENERATOR" -DLLVM_ENABLE_PROJECTS="clang" -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_INSTALL_PREFIX=../install ../src/llvm

$BUILD_CMD
