// RUN: %not %clangxx %cxxflags -O2 %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t 2> %t.log
// RUN: %FileCheck %s < %t.log

#include <easy/jit.h>
#include <easy/options.h>

// CHECK: Invalid bind, placeholder cannot be bound to a formal argument

using namespace std::placeholders;

int foo(float) {
  return 0;
}

int main(int, char** argv) {

  auto foo_ = easy::jit(foo, _2);
  return 0;
}
