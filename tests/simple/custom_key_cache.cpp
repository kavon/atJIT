// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/code_cache.h>

#include <string>
#include <functional>
#include <cstdio>

using namespace std::placeholders;

int add (int a, int b) {
  return a+b;
}

void test_int() {
  easy::Cache<int> C;

  for(int i = 0; i != 6; ++i) {
    auto const &inc = C.jit(i, add, _1, i);

    if(!C.has(i)) {
      printf("code not in cache!\n");
    }

    // CHECK-NOT: code not in cache 
    // CHECK: inc.int(0) is 0
    // CHECK: inc.int(0) is 1
    // CHECK: inc.int(0) is 2
    // CHECK: inc.int(0) is 3
    // CHECK: inc.int(0) is 4
    // CHECK: inc.int(0) is 5

    printf("inc.int(%d) is %d\n", 0, inc(0));
  }
}

void test_string() {
  easy::Cache<std::string> C;

  for(int i = 0; i != 6; ++i) {
    auto const &inc = C.jit(std::to_string(i), add, _1, i);

    if(!C.has(std::to_string(i))) {
      printf("code not in cache!\n");
    }

    // CHECK-NOT: code not in cache 
    // CHECK: inc.str(0) is 0
    // CHECK: inc.str(0) is 1
    // CHECK: inc.str(0) is 2
    // CHECK: inc.str(0) is 3
    // CHECK: inc.str(0) is 4
    // CHECK: inc.str(0) is 5

    printf("inc.str(%d) is %d\n", 0, inc(0));
  }
}

int main() {
  test_int();
  test_string();
  return 0;
}
