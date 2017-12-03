#ifndef STATIC_PASSES
#define STATIC_PASSES

#include <llvm/Pass.h>

namespace easy {
  llvm::Pass* createRegisterBitcodePass();
}

#endif
