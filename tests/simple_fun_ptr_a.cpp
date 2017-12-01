// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: FileCheck %s < %t.out

#include <easy/jit.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

int __attribute__((section("jit"))) b() {
  static int v = 0;
  return v;
}

int __attribute__((section("jit"))) add (int a, int (*f)()) {
  return a+f();
}

int main() {
  easy::FunctionWrapper<int(int)> inc = easy::jit(add, _1, b);

  // CHECK: inc(4) is 4
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 8
  // CHECK: inc(7) is 10
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, inc(v));

  return 0;
}
