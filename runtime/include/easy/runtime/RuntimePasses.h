#ifndef RUNTIME_PASSES
#define RUNTIME_PASSES

#include<llvm/Pass.h>
#include<llvm/ADT/StringRef.h>
#include<easy/runtime/Context.h>
#include<easy/runtime/BitcodeTracker.h>

namespace easy {
  struct ContextAnalysis :
      public llvm::ImmutablePass {

    static char ID;

    ContextAnalysis()
      : llvm::ImmutablePass(ID), C_(nullptr) {}
    ContextAnalysis(Context const &C)
      : llvm::ImmutablePass(ID), C_(&C) {}

    easy::Context const& getContext() const {
      return *C_;
    }

    private:

    easy::Context const *C_;
  };

  struct InlineParameters:
      public llvm::ModulePass {

    static char ID;

    InlineParameters()
      : llvm::ModulePass(ID) {}
    InlineParameters(llvm::StringRef TargetName, GlobalMapping const* Globals)
      : llvm::ModulePass(ID), TargetName_(TargetName), Globals_(Globals) {}

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
      AU.addRequired<ContextAnalysis>();
    }

    bool runOnModule(llvm::Module &M) override;

    private:
    GlobalMapping const* Globals_;
    llvm::StringRef TargetName_;
  };

  llvm::Pass* createContextAnalysisPass(easy::Context const &C);
  llvm::Pass* createInlineParametersPass(llvm::StringRef Name, GlobalMapping const*);
}

#endif
