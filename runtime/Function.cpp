#include <easy/runtime/Function.h>
#include <easy/runtime/RuntimePasses.h>

#include <llvm/Transforms/IPO/PassManagerBuilder.h> 
#include <llvm/IR/LegacyPassManager.h> 
#include <llvm/Support/Host.h> 
#include <llvm/Target/TargetMachine.h> 
#include <llvm/Support/TargetRegistry.h> 
#include <llvm/Analysis/TargetTransformInfo.h> 
#include <llvm/Analysis/TargetLibraryInfo.h> 

using namespace easy;

std::unique_ptr<llvm::TargetMachine> Function::GetTargetMachine(llvm::StringRef Triple) {
  std::string TgtErr;
  llvm::Target const *Tgt = llvm::TargetRegistry::lookupTarget(Triple, TgtErr);

  llvm::StringMap<bool> Features;
  (void)llvm::sys::getHostCPUFeatures(Features);

  std::string FeaturesStr;
  for (auto &&KV : Features) {
    if (KV.getValue()) {
      FeaturesStr += '+';
      FeaturesStr += KV.getKey();
      FeaturesStr += ',';
    }
  }

  return std::unique_ptr<llvm::TargetMachine>(
      Tgt->createTargetMachine(Triple, llvm::sys::getHostCPUName(), 
                               FeaturesStr, llvm::TargetOptions(), llvm::None));
}

void Function::Optimize(llvm::Module& M, const char* Name, const Context& C, int OptLevel, int OptSize) {

  auto Triple = llvm::sys::getProcessTriple();

  llvm::PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;
  Builder.SizeLevel = OptSize;
  Builder.LibraryInfo = new llvm::TargetLibraryInfoImpl(llvm::Triple{Triple});

  std::unique_ptr<llvm::TargetMachine> TM = GetTargetMachine(Triple);

  llvm::legacy::PassManager MPM;
  MPM.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
  MPM.add(easy::createContextAnalysisPass(C));
  MPM.add(easy::createInlineParametersPass(Name));
  Builder.populateModulePassManager(MPM);
  MPM.run(M);
}

std::unique_ptr<llvm::ExecutionEngine> Function::GetEngine(std::unique_ptr<llvm::Module> M) {
  llvm::EngineBuilder ebuilder(std::move(M));
  std::string eeError;

  std::unique_ptr<llvm::ExecutionEngine> EE(ebuilder.setErrorStr(&eeError)
          .setMCPU(llvm::sys::getHostCPUName())
          .setEngineKind(llvm::EngineKind::JIT)
          .setOptLevel(llvm::CodeGenOpt::Level::Aggressive)
          .create());

  if(!EE) {
    // TODO throw easy::exception
    throw std::runtime_error("Failed to create ExecutionEngine.");
  }

  return EE;
}

void Function::MapGlobals(llvm::ExecutionEngine& EE, GlobalMapping* Globals) {
  for(GlobalMapping *GM = Globals; GM->Name; ++GM) {
    EE.addGlobalMapping(GM->Name, (uint64_t)GM->Address);
  }
}

std::unique_ptr<Function> Function::Compile(void *Addr, std::unique_ptr<Context const> C) {

  // TODO: Use C to perform the specialization.

  auto &BT = BitcodeTracker::GetTracker();

  const char* Name;
  GlobalMapping* Globals;
  std::tie(Name, Globals) = BT.getNameAndGlobalMapping(Addr);
  auto Original = BT.getModule(Addr);

  std::unique_ptr<llvm::Module> Clone(llvm::CloneModule(Original));

  Optimize(*Clone, Name, *C, 2, 0);

  std::unique_ptr<llvm::ExecutionEngine> EE = GetEngine(std::move(Clone));

  MapGlobals(*EE, Globals);

  void *Address = (void*)EE->getFunctionAddress(Name);

  return std::unique_ptr<Function>(new Function{Address, std::move(EE)});
}
