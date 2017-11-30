#ifndef RUNTIME_PASSES
#define RUNTIME_PASSES

#include<llvm/Pass.h>
#include<llvm/ADT/StringRef.h>
#include<easy/runtime/Context.h>

namespace easy {
  struct ContextAnalysis :
      public llvm::ImmutablePass {

    static char ID;

    ContextAnalysis(Context const &C)
      : llvm::ImmutablePass(ID), C_(C) {}

    easy::Context const& getContext() const {
      return C_;
    }

    private:

    easy::Context const &C_;
  };

  struct InlineParameters:
      public llvm::ModulePass {

    static char ID;

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
      AU.addRequired<ContextAnalysis>();
    }

    InlineParameters(llvm::StringRef TargetName)
      : llvm::ModulePass(ID), TargetName_(TargetName) {}

    bool runOnModule(llvm::Module &M) override;

    private:

    llvm::StringRef TargetName_;
  };

  llvm::Pass* createContextAnalysisPass(easy::Context const &C);
  llvm::Pass* createInlineParametersPass(llvm::StringRef Name);
}

#endif
