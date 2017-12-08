// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -lpthread -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/jit.h>

#include <future>
#include <functional>
#include <cstdio>

using namespace std::placeholders;

int add (int a, int b) {
  return a+b;
}

int main() {
  auto inc_future = std::async(std::launch::async,
                               [](){ return easy::jit(add, _1, 1);});

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 7
  // CHECK: inc(7) is 8
  auto inc = inc_future.get();
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, inc(v));

  return 0;
}
