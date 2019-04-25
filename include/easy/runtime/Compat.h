#pragma once

// handle some minor API differences
#if LLVM_VERSION_MAJOR == 6

  #define LLVM_DEBUG DEBUG
  #define PASS_MODULE_ARG(M) &(M)

#elif LLVM_VERSION_MAJOR == 8

  #define PASS_MODULE_ARG(M) (M)

#else

#warning "Compatibility with this version of LLVM is unknown!"

#endif
