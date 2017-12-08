// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/jit.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

double add (double a, double b) {
  return a+b;
}

int main() {
  easy::FunctionWrapper<double(double)> inc = easy::jit(add, _1, 1);

  // CHECK: inc(4.00) is 5.00
  // CHECK: inc(5.00) is 6.00
  // CHECK: inc(6.00) is 7.00
  // CHECK: inc(7.00) is 8.00
  for(int v = 4; v != 8; ++v)
    printf("inc(%.2f) is %.2f\n", (double)v, inc(v));

  return 0;
}
