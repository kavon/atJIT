#include <easy/runtime/RuntimePasses.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace easy;

char easy::InlineParameters::ID = 0;

llvm::Pass* easy::createInlineParametersPass(llvm::StringRef Name) {
  return new InlineParameters(Name);
}

bool easy::InlineParameters::runOnModule(llvm::Module &M) {
  Context const &C = getAnalysis<ContextAnalysis>().getContext();
  errs() << "on run on module!\n";
  return false;
}
