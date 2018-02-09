// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/jit.h>

#include <functional>
#include <cstdio>
#include <sstream>
#include <string>

using namespace std::placeholders;

int add (int a, int b) {
  return a+b;
}

int main() {
  auto inc_store = easy::jit(add, _1, 1);

  std::stringstream out;
  inc_store.serialize(out);
  out.flush();

  std::string buffer = out.str();

  assert(buffer.size());
  printf("buffer.size() = %lu\n", buffer.size());

  std::stringstream in(buffer);
  auto inc_load = easy::FunctionWrapper<int(int)>::deserialize(in);

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 6
  // CHECK: inc(6) is 7
  // CHECK: inc(7) is 8
  for(int v = 4; v != 8; ++v)
    printf("inc(%d) is %d\n", v, inc_load(v));

  return 0;
}
