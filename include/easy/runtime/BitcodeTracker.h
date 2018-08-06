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
  std::unordered_map<std::string, void*> NameToAddress;

  public:

  void registerFunction(void* FPtr, const char* Name, GlobalMapping* Globals, const char* Bitcode, size_t BitcodeLen) {
    Functions.emplace(FPtr, FunctionInfo{Name, Globals, Bitcode, BitcodeLen});
    NameToAddress.emplace(Name, FPtr);
  }

  void* getAddress(std::string const &Name);
  const char* getName(void*);
  std::tuple<const char*, GlobalMapping*> getNameAndGlobalMapping(void* FPtr);
  bool hasGlobalMapping(void* FPtr) const;

  /*
   NOTE: on getModule / getModuleWithContext

   "LLVMContext owns and manages the core "global" data of LLVM's core
   infrastructure, including the type and constant uniquing tables.
   LLVMContext itself provides no locking guarantees, so you should be
   careful to have one context per thread."

   Thus, right now, it seems we're stuck with parsing the bitcode into a new
   chunk of memory on each JIT event.

   */

  using ModuleContextPair = std::pair<std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::LLVMContext>>;
  ModuleContextPair getModule(void* FPtr);
  std::unique_ptr<llvm::Module> getModuleWithContext(void* FPtr, llvm::LLVMContext &C);

  // get the singleton object
  static BitcodeTracker& GetTracker();
};

}

#endif
