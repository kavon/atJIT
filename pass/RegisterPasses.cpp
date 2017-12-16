#include "StaticPasses.h"

#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace easy;

static void callback(const PassManagerBuilder &,
                     legacy::PassManagerBase &PM) {
  PM.add(easy::createRegisterBitcodePass());
}

RegisterStandardPasses Register(PassManagerBuilder::EP_OptimizerLast, callback);
RegisterStandardPasses RegisterO0(PassManagerBuilder::EP_EnabledOnOptLevel0, callback);
