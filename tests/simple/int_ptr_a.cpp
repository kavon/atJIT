// RUN: %atjitc   %s -o %t
// RUN: %t "%t.ll" > %t.out
// RUN: %FileCheck %s < %t.out
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll

#include <easy/jit.h>
#include <easy/options.h>

#include <functional>
#include <cstdio>

// verify that the variable 'b' is not loaded, and the addition is performed using its constant value
// CHECK-IR-NOT: inttoptr 
// CHECK-IR-NOT: load i32
// CHECK-IR: add{{.*}}4321 

using namespace std::placeholders;

static int add (int a, int *b) {
  return a+*b;
}

int const b = 4321;

int main(int argc, char** argv) {
  easy::FunctionWrapper<int(int)> inc = easy::jit(add, _1, &b, easy::options::dump_ir(argv[1]));

  // CHECK: inc(4) is 4325
  // CHECK: inc(5) is 4326
  // CHECK: inc(6) is 4327
  // CHECK: inc(7) is 4328
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, inc(v));

  return 0;
}
