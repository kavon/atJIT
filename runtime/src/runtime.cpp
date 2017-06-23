#include "llvm/InitializePasses.h"

#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "llvm/ADT/StringMap.h"


using namespace llvm;

namespace runtime {

class InitNativeTarget {
  public:
  InitNativeTarget() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
  }
};

class ModuleCache {
  LLVMContext Context_;
  StringMap<std::unique_ptr<Module>> Modules_;

  std::unique_ptr<Module>& loadModule(const char* ModuleStr, size_t size) {

    auto KeyPtr = std::make_pair(ModuleStr, nullptr);
    auto Pair = Modules_.insert(KeyPtr);

    if(Pair.second) {
      StringRef BytecodeStr(ModuleStr, size - 1);
      std::unique_ptr<MemoryBuffer> MemoryBuffer(MemoryBuffer::getMemBuffer(BytecodeStr));
      auto ModuleOrErr = parseBitcodeFile(MemoryBuffer->getMemBufferRef(), Context_);

      if(auto EC = ModuleOrErr.takeError()) {
        assert(false && "error loading the module.");
      }
      Pair.first->getValue() = std::move(ModuleOrErr.get());
    }
    return Pair.first->getValue();
  }

  public:

  std::unique_ptr<llvm::Module> getModule(const char* m, size_t size) {
    // TODO this is expensive! could we perform the modifications on the module and re-use it?
    //    the problem is that the ExecEngine take ownership of the module.
    return CloneModule(loadModule(m, size).get());
  }
};

std::unique_ptr<llvm::ExecutionEngine> getExecutionEngine(std::unique_ptr<Module> M) {
  EngineBuilder ebuilder(std::move(M));
  std::string eeError;

  std::unique_ptr<llvm::ExecutionEngine> ee(ebuilder.setErrorStr(&eeError)
          .setMCPU(sys::getHostCPUName())
          .setEngineKind(EngineKind::JIT)
          .setOptLevel(llvm::CodeGenOpt::Level::Aggressive)
          .create());

  if(!ee) {
    errs() << eeError << "\n";
    assert(false && "error creating the execution engine.");
  }

  return ee;
}

static InitNativeTarget init;
static ModuleCache AModuleCache;
// TODO A single active execution engine...
static std::unique_ptr<llvm::ExecutionEngine> ExecEngine;
}

using namespace runtime;

extern "C" void* easy_jit_hook(const char* FunctionName, const char* IR, size_t IRSize) {

  std::unique_ptr<llvm::Module> M(AModuleCache.getModule(IR, IRSize));
  ExecEngine = std::move(getExecutionEngine(std::move(M)));

  std::string JittedFunName = std::string(FunctionName) + "__";
  auto JittedFunAddr = ExecEngine->getFunctionAddress(JittedFunName.c_str());

  return (void*)JittedFunAddr;
}

extern "C" void easy_jit_hook_end(void* JittedFunAddr) {
  ExecEngine = nullptr;
}
