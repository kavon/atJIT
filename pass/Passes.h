#ifndef PASSES
#define PASSES

#include <llvm/Pass.h>

namespace easy {
  llvm::Pass* createRegisterBitcodePass();
}

#endif
