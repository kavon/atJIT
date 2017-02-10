#define define_easy_jit_specialize_param(type) \
  static void easy_jit_specialize_##type##_param( type a ) {}
#define define_easy_jit_specialize_param_ptr(type) \
  static void easy_jit_specialize_##type##_ptr_param( type* a ) {}

#define define_easy_jit_specialize(type) \
  define_easy_jit_specialize_param(type); \
  define_easy_jit_specialize_param_ptr(type);

// enables the jit compilation for one function
static void easy_jit_enabled() {}

// specify a parameter to be inlined
define_easy_jit_specialize(char);
define_easy_jit_specialize(int);
define_easy_jit_specialize(long);
define_easy_jit_specialize(float);
define_easy_jit_specialize(double);
define_easy_jit_specialize_param_ptr(void);
