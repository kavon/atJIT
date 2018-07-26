// RUN: %not %atjitc  -O2  %s -o %t 2> %t.log
// RUN: %FileCheck %s < %t.log

#include <easy/jit.h>
#include <easy/options.h>

// CHECK: easy::jit: not providing enough argument to actual call

using namespace std::placeholders;
using namespace tuned_param;

int foo(int, int) {
  return 0;
}

int main(int, char** argv) {

  auto foo_ = easy::jit(foo, IntRange(-5, 5));
  return 0;
}
