#include "declare.h"

using namespace llvm;

namespace declare {

const char* JitHook::Name = "easy_jit_hook";
const char* JitHookEnd::Name = "easy_jit_hook_end";

FunctionType* JitHook::getType(LLVMContext &C) {
  Type* I8Ptr = Type::getInt8PtrTy(C);
  Type* I64 = Type::getInt64Ty(C);
  return FunctionType::get(I8Ptr, {I8Ptr, I8Ptr, I64}, false);
}

FunctionType* JitHookEnd::getType(LLVMContext &C) {
  Type* VoidTy = Type::getVoidTy(C);
  Type* I8Ptr = Type::getInt8PtrTy(C);
  return FunctionType::get(VoidTy, {I8Ptr}, false);
}

}
