#include <easy/runtime/Function.h>
#include <easy/runtime/RuntimePasses.h>
#include <easy/runtime/LLVMHolderImpl.h>
#include <easy/exceptions.h>

#include <llvm/Transforms/IPO/PassManagerBuilder.h> 
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/Host.h> 
#include <llvm/Target/TargetMachine.h> 
#include <llvm/Support/TargetRegistry.h> 
#include <llvm/Analysis/TargetTransformInfo.h> 
#include <llvm/Analysis/TargetLibraryInfo.h> 
#include <llvm/Support/FileSystem.h>

using namespace easy;

namespace easy {
  DefineEasyException(ExecutionEngineCreateError, "Failed to create execution engine for:");
  DefineEasyException(CouldNotOpenFile, "Failed to file to dump intermediate representation.");
}

Function::Function(void* Addr, std::unique_ptr<LLVMHolder> H)
  : Address(Addr), Holder(std::move(H)) {
}

std::unique_ptr<llvm::TargetMachine> Function::GetHostTargetMachine() {
  std::unique_ptr<llvm::TargetMachine> TM(llvm::EngineBuilder().selectTarget());
  return TM;
}

void Function::Optimize(llvm::Module& M, const char* Name, const easy::Context& C, unsigned OptLevel, unsigned OptSize) {

  llvm::Triple Triple{llvm::sys::getProcessTriple()};

  llvm::PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;
  Builder.SizeLevel = OptSize;
  Builder.LibraryInfo = new llvm::TargetLibraryInfoImpl(Triple);
  Builder.Inliner = llvm::createFunctionInliningPass(OptLevel, OptSize, false);

  std::unique_ptr<llvm::TargetMachine> TM = GetHostTargetMachine();
  assert(TM);
  TM->adjustPassManager(Builder);

  llvm::legacy::PassManager MPM;
  MPM.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
  MPM.add(easy::createContextAnalysisPass(C));
  MPM.add(easy::createInlineParametersPass(Name));
  Builder.populateModulePassManager(MPM);
  MPM.add(easy::createDevirtualizeConstantPass(Name));
  Builder.populateModulePassManager(MPM);

  MPM.run(M);
}

std::unique_ptr<llvm::ExecutionEngine> Function::GetEngine(std::unique_ptr<llvm::Module> M, const char *Name) {
  llvm::EngineBuilder ebuilder(std::move(M));
  std::string eeError;

  std::unique_ptr<llvm::ExecutionEngine> EE(ebuilder.setErrorStr(&eeError)
          .setMCPU(llvm::sys::getHostCPUName())
          .setEngineKind(llvm::EngineKind::JIT)
          .setOptLevel(llvm::CodeGenOpt::Level::Aggressive)
          .create());

  if(!EE) {
    throw easy::ExecutionEngineCreateError(Name);
  }

  return EE;
}

void Function::MapGlobals(llvm::ExecutionEngine& EE, GlobalMapping* Globals) {
  for(GlobalMapping *GM = Globals; GM->Name; ++GM) {
    EE.addGlobalMapping(GM->Name, (uint64_t)GM->Address);
  }
}

std::unique_ptr<Function> Function::Compile(void *Addr, easy::Context const& C) {

  auto &BT = BitcodeTracker::GetTracker();

  const char* Name;
  GlobalMapping* Globals;
  std::tie(Name, Globals) = BT.getNameAndGlobalMapping(Addr);

  std::unique_ptr<llvm::Module> M;
  std::unique_ptr<llvm::LLVMContext> Ctx;
  std::tie(M, Ctx) = BT.getModule(Addr);

  unsigned OptLevel;
  unsigned OptSize;
  std::tie(OptLevel, OptSize) = C.getOptLevel();

  Optimize(*M, Name, C, OptLevel, OptSize);

  WriteOptimizedToFile(*M, C.getDebugFile());

  std::unique_ptr<llvm::ExecutionEngine> EE = GetEngine(std::move(M), Name);

  MapGlobals(*EE, Globals);

  void *Address = (void*)EE->getFunctionAddress(Name);

  std::unique_ptr<LLVMHolder> Holder(new easy::LLVMHolderImpl{std::move(EE), std::move(Ctx)});
  return std::unique_ptr<Function>(new Function(Address, std::move(Holder)));
}

void Function::WriteOptimizedToFile(llvm::Module const &M, std::string const& File) {
  if(File.empty())
    return;
  std::error_code Error;
  llvm::raw_fd_ostream Out(File, Error, llvm::sys::fs::F_None);

  if(Error)
    throw CouldNotOpenFile(Error.message());

  Out << M;
}
