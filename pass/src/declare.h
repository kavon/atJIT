#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>

#ifndef DECLARE
#define DECLARE

#define declare(name) struct name { \
                        static const char* Name; \
                        static llvm::FunctionType* getType(llvm::LLVMContext &); \
                      };

namespace declare {
declare(JitHook)
declare(JitHookEnd)
}

template<class T>
llvm::Function* Declare(llvm::Module &M) {
  if(llvm::Function* F = M.getFunction(T::Name))
    return F;

  return llvm::Function::Create(T::getType(M.getContext()),
                                llvm::Function::ExternalLinkage,
                                T::Name, &M);
}

#endif
