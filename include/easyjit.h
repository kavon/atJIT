#define define_easy_jit_specialize_param(type) \
  static void easy_jit_specialize_##type##_param( type a ) {}

#define define_easy_jit_specialize(type) \
  define_easy_jit_specialize_param(type);

// enables the jit compilation for one function
//
// in the example bellow, foo gets compiled at runtime with optimization level 2
// (-O2), and specialized for parameter 'a'
//
// int foo(int a, double b) {
//   easy_jit_enabled(2, a);
//   ...
// }
static void easy_jit_enabled(int optlevel, ...) {}
