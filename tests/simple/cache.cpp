// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/code_cache.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

int add (int a, int b) {
  return a+b;
}

int main() {
  easy::Cache<> C;

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 7
  // CHECK: inc(7) is 8

  for(int i = 0; i != 16; ++i) {
    auto const &inc = C.jit(add, _1, 1);

    if(!C.has(add, _1, 1)) {
      printf("code not in cache!\n");
      return -1;
    }

    for(int v = 4; v != 8; ++v)
      printf("inc(%d) is %d\n", v, inc(v));
  }

  return 0;
}
