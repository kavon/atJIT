#ifndef BITCODETRACKER
#define BITCODETRACKER

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <unordered_map>
#include <memory>

namespace easy {

struct GlobalMapping {
  const char* Name;
  void* Address;
};

struct FunctionInfo {
  const char* Name;
  GlobalMapping* Globals;
  const char* Bitcode;
  size_t BitcodeLen;

  FunctionInfo(const char* N, GlobalMapping* G, const char* B, size_t BL)
    : Name(N), Globals(G), Bitcode(B), BitcodeLen(BL)
  { }
};

class BitcodeTracker {

  // map function to all the info required for jit compilation
  std::unordered_map<void*, FunctionInfo> Functions;

  public:

  void registerFunction(void* FPtr, const char* Name, GlobalMapping* Globals, const char* Bitcode, size_t BitcodeLen) {
    Functions.emplace(FPtr, FunctionInfo{Name, Globals, Bitcode, BitcodeLen});
  }

  std::tuple<const char*, GlobalMapping*> getNameAndGlobalMapping(void* FPtr);
  bool hasGlobalMapping(void* FPtr) const;

  using ModuleContextPair = std::pair<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::LLVMContext>>;
  ModuleContextPair getModule(void* FPtr);
  std::unique_ptr<llvm::Module> getModuleWithContext(void* FPtr, llvm::LLVMContext &C);

  // get the singleton object
  static BitcodeTracker& GetTracker();
};

}

extern "C" void easy_register(void* FPtr, const char* Name, easy::GlobalMapping* Globals, const char* Bitcode, size_t BitcodeLen);

#endif
