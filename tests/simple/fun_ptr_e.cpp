// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t "%t.ll" > %t.out
// RUN: %FileCheck %s < %t.out
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll

#include <easy/jit.h>
#include <easy/options.h>

#include <functional>
#include <cstdio>

// only one function, without any call
// CHECK-IR-NOT: call{{ }}


using namespace std::placeholders;

static void foo(void* dat) {
  (*(int*)dat) += 1;
}
static void bar(void* dat) {
  (*(int*)dat) += 2;
}

static void map (void* data, unsigned nmemb, unsigned size, void (*f)(void*)) {
  for(unsigned i = 0; i < nmemb; ++i)
    f((char*)data + i * size);
}

int main(int argc, char** argv) {

  void (*(come_and_get_some[2]))(void*dat);
  come_and_get_some[0] = foo;
  come_and_get_some[1] = bar;

  easy::FunctionWrapper<void(void*, unsigned, unsigned)> map_w = easy::jit(map, _1, _2, _3, come_and_get_some[argc?1:0], easy::options::dump_ir(argv[1]));

  int data[] = {1,2,3,4};
  map_w(data, sizeof(data)/sizeof(data[0]), sizeof(data[0]));

  // CHECK: data[0] is 3
  // CHECK: data[1] is 4
  // CHECK: data[2] is 5
  // CHECK: data[3] is 6
  for(int v = 0; v != 4; ++v)
    printf("data[%d] is %d\n", v, data[v]);

  return 0;
}
