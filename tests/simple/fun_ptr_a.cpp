// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t "%t.ll" > %t.out
// RUN: %FileCheck %s < %t.out
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll

#include <easy/jit.h>
#include <easy/options.h>

#include <functional>
#include <cstdio>

// only one function
// reading from a global variable
// CHECK-IR: @[[GLOBAL:.+]] = external
// CHECK-IR: define
// CHECK-IR-NOT: define
// CHECK-IR-NOT: br
// CHECK-IR: load{{.*}}[[GLOBAL]]
// CHECK-IR: add
// CHECK-IR: store{{.*}}[[GLOBAL]]
// CHECK-IR: ret


using namespace std::placeholders;

static int bubu() {
  static int v = 0;
  return v++;
}

static int add (int a, int (*f)()) {
  return a+f();
}

int main(int argc, char** argv) {
  easy::FunctionWrapper<int(int)> inc = easy::jit(add, _1, bubu, easy::options::dump_ir(argv[1]));

  // CHECK: inc(4) is 4
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 8
  // CHECK: inc(7) is 10
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, inc(v));

  return 0;
}
