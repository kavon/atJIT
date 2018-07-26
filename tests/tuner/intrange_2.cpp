// RUN: %atjitc  -O2  %s -o %t
// RUN: %t "%t.ll"
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll
// RUN: %FileCheck --check-prefix=CHECK-IR-BEFOREJIT %s < %t.ll.before

#include <tuner/driver.h>
#include <tuner/param.h>

#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cinttypes>

#include <chrono>
#include <thread>

using namespace std::placeholders;
using namespace tuned_param;

int dispatch(int choice) {
  if (choice <= 0) {
    return 31337;
  } else {
    return 44444;
  }
}

int main(int argc, char** argv) {

  {
    // test if the JIT compilation properly
    // inlines the IntRange's value as a constant,
    // which is then used to simplify the dispatcher completely.
    // this also tests the default setting on IntRange.
    auto F = easy::jit(dispatch, IntRange(0, 1, 1), easy::options::dump_ir(argv[1]));

    // CHECK-IR:      define i32 @_Z8dispatchi() local_unnamed_addr #0 {
    // CHECK-IR-NEXT:   ret i32 44444
    // CHECK-IR-NEXT: }

    // CHECK-IR-BEFOREJIT: i32 31337
  }

  return 0;
}
