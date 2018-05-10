// RUN: %clangxx %cxxflags %include_flags %ld_flags %s -Xclang -load -Xclang %lib_pass -o %t
// RUN: %t 8 1 2 3 4 5 6 7 8 %t.ll > %t.out
// RUN: %FileCheck %s < %t.out
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll
//
// CHECK: 72
//
// only one function in the final IR
// CHECK-IR: define 
// CHECK-IR-NOT: define 

#include <easy/jit.h>

#include <cstdlib>
#include <cstdio>
#include <vector>

using namespace std::placeholders;

int mul(int a, int b) {
  return a*b;
}

int accumulate(std::vector<int> const &vec, int acum, int(*fun)(int)) {
  int a = acum;
  for(int e : vec)
    a += fun(e);
  return a;
}

int main(int argc, char** argv) {

  int n = atoi(argv[1]); 

  // read input
  std::vector<int> vec;
  for(int i = 0; i != n; ++i)
    vec.emplace_back(atoi(argv[i+2]));

  // generate code
  easy::FunctionWrapper<int(int)> mul_by_two = easy::jit(mul, _1, 2);

  static_assert(easy::is_function_wrapper<decltype(mul_by_two)>::value, "Value not detected as function wrapper!");
  static_assert(easy::is_function_wrapper<decltype(mul_by_two)&>::value, "Reference not detected as function wrapper!");
  static_assert(easy::is_function_wrapper<decltype(mul_by_two)&&>::value, "RReference not detected as function wrapper!");

  easy::FunctionWrapper<int(std::vector<int> const&)> mul_vector_by_two = easy::jit(accumulate, _1, 0, mul_by_two, 
                                                                                    easy::options::dump_ir(argv[argc-1]));

  // kernel!
  int result = mul_vector_by_two(vec);

  // output
  printf("%d\n", result);

  return 0;
}
