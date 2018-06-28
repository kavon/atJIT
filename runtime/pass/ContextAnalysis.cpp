#include <easy/runtime/RuntimePasses.h>

using namespace llvm;
using namespace easy;

char easy::ContextAnalysis::ID = 0;

llvm::Pass* easy::createContextAnalysisPass(std::shared_ptr<easy::Context> C) {
  return new ContextAnalysis(std::move(C));
}

static RegisterPass<easy::ContextAnalysis> X("", "", true, true);
