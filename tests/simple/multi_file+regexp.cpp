// RUN: %clangxx %cxxflags %include_flags %s -DMAIN -c -o %t.main.o
// RUN: %clangxx %cxxflags %include_flags %s -Xclang -load -Xclang %lib_pass -DLIB -c -o %t.lib.o -mllvm -easy-regex="add"
// RUN: %clangxx %ld_flags %t.main.o %t.lib.o -o %t 
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#ifdef LIB

extern "C" int add (int a, int b) {
  return a+b;
}

#endif

#ifdef MAIN

#include <easy/jit.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

extern "C" int add (int a, int b);

int main() {
  easy::FunctionWrapper<int(int)> inc = easy::jit(add, _1, 1);

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 7
  // CHECK: inc(7) is 8
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, inc(v));

  return 0;
}

#endif
