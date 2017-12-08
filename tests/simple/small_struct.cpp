// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/jit.h>

#include <functional>
#include <cstdio>

using namespace std::placeholders;

struct Point {
  int x;
  int y;
};

int add (Point a, Point b) {
  return a.x+b.x+a.y+b.y;
}

int main() {
  easy::FunctionWrapper<int(Point)> inc = easy::jit(add, _1, Point{1,1});

  // CHECK: inc(4,4) is 10
  // CHECK: inc(5,5) is 12
  // CHECK: inc(6,6) is 14
  // CHECK: inc(7,7) is 16
  for(int v = 4; v != 8; ++v)
    printf("inc(%d,%d) is %d\n", v, v, inc(Point{v,v}));

  return 0;
}
