// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t "%t.ll" > %t.out
// RUN: %FileCheck %s < %t.out
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll

#include <easy/jit.h>
#include <easy/options.h>

#include <functional>
#include <cstdio>

// CHECK-IR-NOT: inttoptr

using namespace std::placeholders;

static int add (int a, int *b) {
  return a+*b;
}

int const b = 1;

int main(int argc, char** argv) {
  easy::FunctionWrapper<int(int)> inc = easy::jit(add, _1, &b, easy::options::dump_ir(argv[1]));

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 7
  // CHECK: inc(7) is 8
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, inc(v));

  return 0;
}
