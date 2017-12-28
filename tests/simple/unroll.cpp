// RUN: %clangxx %cxxflags -O2 %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t "%t.ll" > %t.out
// RUN: %FileCheck %s < %t.out
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll

#include <easy/jit.h>
#include <easy/options.h>

#include <functional>
#include <cstdio>

// only one function
// with only one block (~no branch)
// CHECK-IR: define
// CHECK-IR-NOT: define
// CHECK-IR-NOT: br
// CHECK-IR: ret

using namespace std::placeholders;

int dot(std::vector<int> a, std::vector<int> b) {
  int x = 0;
  for(size_t i = 0, n = a.size(); i != n; ++i) {
    x += a[i]*b[i];
  }
  return x;
}

int main(int, char** argv) {

  std::vector<int> a = {1,2,3,4},
                   b = {4,3,2,1};

  auto dot_a = easy::jit(dot, a, _1, easy::options::dump_ir(argv[1]));
  int x = dot_a(b);

  // CHECK: dot is 20
  printf("dot is %d\n", x);

  return 0;
}
