#pragma once

#include<llvm/Pass.h>
#include<llvm/ADT/StringRef.h>
#include<easy/runtime/Context.h>

namespace easy {
  struct ContextAnalysis :
      public llvm::ImmutablePass {

    static char ID;

    ContextAnalysis()
      : llvm::ImmutablePass(ID), C_(nullptr) {}
    ContextAnalysis(std::shared_ptr<easy::Context> C)
      : llvm::ImmutablePass(ID), C_(std::move(C)) {}

    easy::Context const* getContext() const {
      return C_.get();
    }

    private:

    std::shared_ptr<easy::Context> C_;
  };

  struct InlineParameters:
      public llvm::ModulePass {

    static char ID;

    InlineParameters()
      : llvm::ModulePass(ID) {}
    InlineParameters(llvm::StringRef TargetName)
      : llvm::ModulePass(ID), TargetName_(TargetName) {}

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
      AU.addRequired<ContextAnalysis>();
    }

    bool runOnModule(llvm::Module &M) override;

    private:
    llvm::StringRef TargetName_;
  };

  struct DevirtualizeConstant :
      public llvm::FunctionPass {

    static char ID;

    DevirtualizeConstant()
      : llvm::FunctionPass(ID) {}
    DevirtualizeConstant(llvm::StringRef TargetName)
      : llvm::FunctionPass(ID), TargetName_(TargetName) {}

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
      AU.addRequired<ContextAnalysis>();
    }

    bool runOnFunction(llvm::Function &F) override;

    private:
    llvm::StringRef TargetName_;
  };

  llvm::Pass* createContextAnalysisPass(std::shared_ptr<easy::Context> C);
  llvm::Pass* createInlineParametersPass(llvm::StringRef Name);
  llvm::Pass* createDevirtualizeConstantPass(llvm::StringRef Name);
}
