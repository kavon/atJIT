// RUN: %clangxx %cxxflags -O2 %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t "%t.ll" > %t.out
// RUN: %FileCheck %s < %t.out
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll

#include <easy/jit.h>
#include <easy/options.h>

#include <functional>
#include <cstdio>

// only one function
// reading from a global variable
// CHECK-IR-NOT: = tail call


using namespace std::placeholders;

struct Foo {
  virtual int doit() { return 1; }
  virtual ~Foo() = default;
};

struct Bar : Foo {
  int doit() override  { return 2; }
};

int doit(Foo* f) {
  return f->doit();
}

int main(int argc, char** argv) {
  Foo* f = nullptr;
  if(argc == 1)
    f = new Foo();
  else
    f = new Bar();

  easy::FunctionWrapper<int(void)> easy_doit = easy::jit(doit, f, easy::options::dump_ir(argv[1]));

  // CHECK: doit() is 2
  printf("doit() is %d\n", easy_doit());

  delete f;

  return 0;
}
