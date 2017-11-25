// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -o %t
// RUN: %t > %t.out
// RUN: FileCheck %s < %t.out

#include <easy/jit.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

int add (int a, int b) {
  return a+b;
}

int main() {
  auto inc = easy::jit(add, _1, 1);
  int(*RawPtr)(int) = inc.getRawPointer<int(int)>();

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 7
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, RawPtr(v));

  return 0;
}
