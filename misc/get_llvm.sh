#!/bin/bash

git clone https://github.com/llvm-mirror/llvm.git -b release_50 llvm
git clone https://github.com/llvm-mirror/clang.git -b release_50 llvm/tools/clang

mkdir -p llvm/_build
cd llvm/_build

cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_RTTI=1 -DLLVM_CCACHE_BUILD=1 -DLLVM_ENABLE_ASSERTIONS=1 -DLLVM_TARGETS_TO_BUILD=X86 -DBUILD_SHARED_LIBS=1 -G Ninja 
ninja 
