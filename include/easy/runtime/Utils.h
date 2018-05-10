#ifndef UTILS
#define UTILS

#include <string>
#include <memory>

namespace llvm {
  class LLVMContext;
  class Module;
  class Function;
}

namespace easy {

std::string GetEntryFunctionName(llvm::Module const &M);
void MarkAsEntry(llvm::Function &F);
void UnmarkEntry(llvm::Module &M);

std::unique_ptr<llvm::Module>
CloneModuleWithContext(llvm::Module const &LM, llvm::LLVMContext &C);

}

#endif
