#ifndef UTILS
#define UTILS

#include <string>

namespace llvm {
  class Module;
  class Function;
}

namespace easy {

std::string GetEntryFunctionName(llvm::Module const &M);
void MarkAsEntry(llvm::Function &F);

}

#endif
