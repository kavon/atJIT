#ifndef BITCODETRACKER
#define BITCODETRACKER

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <unordered_map>
#include <memory>
#include <mutex>

namespace easy {

struct GlobalMapping {
  const char* Name;
  void* Address;
};

struct FunctionJitContext {
  std::unique_ptr<llvm::LLVMContext> Context;
  std::unique_ptr<llvm::Module> Module;

  FunctionJitContext(std::unique_ptr<llvm::LLVMContext> C)
    : Context(std::move(C)) { }
};

struct FunctionInfo {
  // static
  const char* Name;
  GlobalMapping* Globals;
  const char* Bitcode;
  size_t BitcodeLen;

  // dynamic
  std::unique_ptr<std::mutex> CachedLock;
  std::shared_ptr<FunctionJitContext> FJC;

  FunctionInfo(const char* N, GlobalMapping* G, const char* B, size_t BL)
    : Name(N), Globals(G), Bitcode(B), BitcodeLen(BL), CachedLock(new std::mutex())
  { }
};

class BitcodeTracker {

  // map function to all the info required for jit compilation
  std::unordered_map<void*, FunctionInfo> Functions;

  public:

  void registerFunction(void* FPtr, const char* Name, GlobalMapping* Globals, const char* Bitcode, size_t BitcodeLen) {
    llvm::errs() << "register " << FPtr << " " << Name << "\n";
    Functions.emplace(FPtr, FunctionInfo{Name, Globals, Bitcode, BitcodeLen});
  }

  std::tuple<const char*, GlobalMapping*> getNameAndGlobalMapping(void* FPtr);
  llvm::Module* getModule(void* FPtr);

  // get the singleton object
  static BitcodeTracker& GetTracker();
};

}

#endif
