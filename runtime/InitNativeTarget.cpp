#include <llvm/Support/TargetSelect.h>

#include <llvm/LinkAllIR.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/ExecutionEngine/MCJIT.h>

using namespace llvm;

namespace {
class InitNativeTarget {
  public:
  InitNativeTarget() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
  }
} Init;
}
