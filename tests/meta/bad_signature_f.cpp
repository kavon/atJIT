// RUN: %not %atjitc  -O2  %s -o %t 2> %t.log
// RUN: %FileCheck %s < %t.log

#include <easy/jit.h>
#include <easy/options.h>

// CHECK: An easy::jit option is expected

using namespace std::placeholders;
using namespace tuned_param;

int foo(int) {
  return 0;
}

int main(int, char** argv) {

  auto foo_ = easy::jit(foo, IntRange(-5, 5), IntRange(-5, 5));
  return 0;
}
