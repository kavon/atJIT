#include "llvm/InitializePasses.h"

#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/Support/TargetRegistry.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/ADT/StringMap.h"

#include <stdarg.h>

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
      auto ModuleOrErr =
          parseBitcodeFile(MemoryBuffer->getMemBufferRef(), Context_);

      if (auto EC = ModuleOrErr.takeError()) {
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

std::unique_ptr<llvm::ExecutionEngine> getExecutionEngine(std::unique_ptr<Module> M, int OptLevel) {
  // create target machine, once
  auto TripleStr = sys::getProcessTriple();

  std::string TgtErr;
  Target const *Tgt = TargetRegistry::lookupTarget(TripleStr, TgtErr);

  TargetOptions TO;
  TO.UnsafeFPMath = 1;
  TO.NoInfsFPMath = 1;
  TO.NoNaNsFPMath = 1;

  StringMap<bool> Features;
  (void)sys::getHostCPUFeatures(Features);
  std::string FeaturesStr;
  for (auto &&KV : Features) {
    if (KV.getValue()) {
      FeaturesStr += '+';
      FeaturesStr += KV.getKey();
      FeaturesStr += ',';
    }
  }

  auto *TM_ = Tgt->createTargetMachine(TripleStr,
                                       sys::getHostCPUName(), // cpu
                                       FeaturesStr, TO, None

                                       );

  legacy::FunctionPassManager FPM(M.get());
  legacy::PassManager MPM;

  PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;
  Builder.SizeLevel = 0;
  Builder.LoopVectorize = true;
  Builder.BBVectorize = true;
  Builder.SLPVectorize = true;
  Builder.LoadCombine = true;
  Builder.DisableUnrollLoops = false;
#ifndef NDEBUG
  llvm::setCurrentDebugType("loop-vectorize");
#endif

  Builder.LibraryInfo = new TargetLibraryInfoImpl(Triple{TripleStr});

  MPM.add(createTargetTransformInfoWrapperPass(TM_->getTargetIRAnalysis()));
  FPM.add(createTargetTransformInfoWrapperPass(TM_->getTargetIRAnalysis()));

  Builder.populateFunctionPassManager(FPM);
  Builder.populateModulePassManager(MPM);
  FPM.doInitialization();
  for (Function &F : *M.get())
    FPM.run(F);
  FPM.doFinalization();
  MPM.run(*M.get());

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

extern "C" void *easy_jit_hook(const char *FunctionName, const char *IR,
                               size_t IRSize, ...) {
  va_list ap;
  va_start(ap, IRSize);


  uint32_t optlevel = va_arg(ap, uint32_t);

  std::unique_ptr<llvm::Module> M(AModuleCache.getModule(IR, IRSize));
  std::string JittedFunName = std::string(FunctionName) + "__";

  Function *F = M->getFunction(JittedFunName);

  uint32_t next = va_arg(ap, uint32_t);

  int argcount = 0;
  for (auto &Arg : F->args()) {
    if (argcount == next) {
      uint64_t val = va_arg(ap, uint64_t);

      Type* I64 = Type::getInt64Ty(F->getContext());
      Constant* C = ConstantInt::get(I64, val, false);

      size_t ArgBitWidth = Arg.getType()->getPrimitiveSizeInBits();
      if(ArgBitWidth < 64) {
        C = ConstantExpr::getTrunc(C, IntegerType::get(F->getContext(), ArgBitWidth));
      }

      if(Arg.getType()->isFloatingPointTy()) {
        C = ConstantExpr::getUIToFP(C, Arg.getType());
      } else if (Arg.getType()->isPointerTy()) {
        C = ConstantExpr::getIntToPtr(C, Arg.getType());
      }

      Arg.replaceAllUsesWith(C);
      next = va_arg(ap, uint32_t);
    }
    ++argcount;
  }
  va_end(ap);

  ExecEngine = std::move(getExecutionEngine(std::move(M), optlevel));

  auto JittedFunAddr = ExecEngine->getFunctionAddress(JittedFunName.c_str());

  return (void*)JittedFunAddr;
}

extern "C" void easy_jit_hook_end(void* JittedFunAddr) {
  ExecEngine = nullptr;
}
