#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SetVector.h>

#ifndef IDENTIFY
#define IDENTIFY

namespace llvm {
  class Value;
  class Function;
  class Module;
}

using Values = llvm::SmallVector<llvm::Value*, 4>;
using FunToInlineMap = llvm::MapVector<llvm::Function*, Values>;

FunToInlineMap GetFunctionsToJit(llvm::Module &M);
llvm::SmallVector<llvm::Function*, 8> GetFunctions(FunToInlineMap &);

#endif
