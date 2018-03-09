#ifndef LLVMHOLDER_IMPL
#define LLVMHOLDER_IMPL

#include <easy/runtime/LLVMHolder.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace easy {
class LLVMHolderImpl : public easy::LLVMHolder {
  public:

  std::unique_ptr<llvm::LLVMContext> Context_;
  std::unique_ptr<llvm::ExecutionEngine> Engine_;
  llvm::Module* M_; // the execution engine has the ownership

  LLVMHolderImpl(std::unique_ptr<llvm::ExecutionEngine> EE, std::unique_ptr<llvm::LLVMContext> C, llvm::Module* M)
    : Context_(std::move(C)), Engine_(std::move(EE)), M_(M) {
  }

  virtual ~LLVMHolderImpl() = default;
};
}

#endif
