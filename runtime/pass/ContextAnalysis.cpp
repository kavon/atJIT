#include <easy/runtime/RuntimePasses.h>

using namespace llvm;
using namespace easy;

char easy::ContextAnalysis::ID = 0;

llvm::Pass* easy::createContextAnalysisPass(easy::Context const &C) {
  return new ContextAnalysis(C);
}

static RegisterPass<easy::ContextAnalysis> X("", "", true, true);
