#ifndef LLVMHOLDER_IMPL
#define LLVMHOLDER_IMPL

#include <easy/runtime/LLVMHolder.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace easy {
class LLVMHolderImpl : public easy::LLVMHolder {
  std::unique_ptr<llvm::LLVMContext> Context;
  std::unique_ptr<llvm::ExecutionEngine> Engine;

  public:
  LLVMHolderImpl(std::unique_ptr<llvm::ExecutionEngine> EE, std::unique_ptr<llvm::LLVMContext> C)
    : Context(std::move(C)), Engine(std::move(EE)) {
  }

  virtual ~LLVMHolderImpl() = default;
};
}

#endif
