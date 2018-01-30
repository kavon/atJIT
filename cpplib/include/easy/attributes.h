#ifndef NOINLINE
#define NOINLINE

#define CI_SECTION "compiler-interface"
#define JIT_SECTION "easy-jit"

// mark functions in the easy::jit interface as no inline.
// it's easier for the pass to find the original functions to be jitted.
#define EASY_JIT_COMPILER_INTERFACE \
  __attribute__((noinline)) __attribute__((section(CI_SECTION)))

#define EASY_JIT_EXPOSE \
  __attribute__((section(JIT_SECTION)))

#endif // NOINLINE
