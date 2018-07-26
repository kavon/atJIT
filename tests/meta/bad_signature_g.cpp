// RUN: %not %atjitc  -O2  %s -o %t 2> %t.log
// RUN: %FileCheck %s < %t.log

#include <easy/jit.h>
#include <easy/options.h>

// CHECK: atJIT tunable parameter's underlying type is mismatched

using namespace std::placeholders;
using namespace tuned_param;

// the type must match even in sign
int foo(unsigned int) {
  return 0;
}

int main(int, char** argv) {

  auto foo_ = easy::jit(foo, IntRange(-5, 5));
  return 0;
}
