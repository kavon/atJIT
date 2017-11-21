#ifndef FUNCTION
#define FUNCTION

#include <easy/runtime/Context.h>
#include <easy/runtime/Function.h>

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
    Functions.emplace(FPtr, FunctionInfo{Name, Globals, Bitcode, BitcodeLen});
  }

  std::tuple<const char*, GlobalMapping*> getNameAndGlobalMapping(void* FPtr);
  llvm::Module* getModule(void* FPtr); 

  // get the singleton object
  static BitcodeTracker& GetTracker(); 
};

class Function {

  // do not reorder the fields and do not add virtual methods!
  void* Address; 

  std::unique_ptr<llvm::ExecutionEngine> Engine;

  public:

  Function(void* Addr, std::unique_ptr<llvm::ExecutionEngine> EE) 
    : Address(Addr), Engine(std::move(EE)) {
  }

  static std::unique_ptr<Function> Compile(const char* Name, GlobalMapping* Globals, llvm::Module* Original, std::unique_ptr<Context> C) {

    std::unique_ptr<llvm::Module> Clone(llvm::CloneModule(Original));

    Optimize(*Clone, 2, 0);

    std::unique_ptr<llvm::ExecutionEngine> EE = GetEngine(std::move(Clone));

    MapGlobals(*EE, Globals);

    void *Address = (void*)EE->getFunctionAddress(Name);

    return std::unique_ptr<Function>(new Function{Address, std::move(EE)});
  }

  private:

  static std::unique_ptr<llvm::TargetMachine> GetTargetMachine(llvm::StringRef Triple); 
  static void Optimize(llvm::Module& M, int OptLevel = 0, int OptSize = 0); 
  static std::unique_ptr<llvm::ExecutionEngine> GetEngine(std::unique_ptr<llvm::Module> M); 
  static void MapGlobals(llvm::ExecutionEngine& EE, GlobalMapping* Globals);
};

}

#endif
