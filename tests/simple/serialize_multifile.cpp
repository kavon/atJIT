// RUN: %clangxx %cxxflags %include_flags %s -DMAIN -c -o %t.main.o
// RUN: %clangxx %cxxflags %include_flags %s -Xclang -load -Xclang %lib_pass -DLIB -c -o %t.lib.o -mllvm -easy-export="add"
// RUN: %clangxx %ld_flags %t.main.o %t.lib.o -o %t 
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <easy/jit.h>

#include <functional>
#include <cstdio>
#include <sstream>
#include <string>

using namespace std::placeholders;

#ifdef LIB

static int var = 0;

static int add (int a, int b) {
  return a+b+(var++);
}

std::string get_add(int b) {
  auto inc_store = easy::jit(add, _1, 1);

  std::ostringstream out;
  inc_store.serialize(out);
  out.flush();

  return out.str();
}

#endif

#ifdef MAIN

std::string get_add(int b);

int main() {

  std::string bitcode = get_add(1);
  std::istringstream in(bitcode);
  auto inc_load = easy::FunctionWrapper<int(int)>::deserialize(in);

  // CHECK: inc(4) is 5
  // CHECK: inc(5) is 7
  // CHECK: inc(6) is 9
  // CHECK: inc(7) is 11
  for(int v = 4; v != 8; ++v) {
    printf("inc(%d) is %d\n", v, inc_load(v));
  }

  return 0;
}

#endif
