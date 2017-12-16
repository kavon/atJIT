// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/jit.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

int add (int a, int b) {
  if(a == 8)
    throw std::runtime_error{"an expected error occured"};
  return a+b;
}

int main() {
  easy::FunctionWrapper<int(int)> inc = easy::jit(add, _1, 1);

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 7
  // CHECK: inc(7) is 8
  // CHECK: inc(8) is exception: an expected error occured
  // CHECK: inc(9) is 10
  for(int v = 4; v != 10; ++v) {
    try {
      printf("inc(%d) is %d\n", v, inc(v));
    } catch(std::runtime_error &e) {
      printf("inc(%d) is exception: %s\n", v, e.what());
    }
  }

  return 0;
}
