#include "easy/runtime/Function.h"

#include <llvm/Bitcode/BitcodeReader.h> 
#include <llvm/Transforms/IPO/PassManagerBuilder.h> 
#include <llvm/IR/LegacyPassManager.h> 
#include <llvm/Support/Host.h> 
#include <llvm/Target/TargetMachine.h> 
#include <llvm/Support/TargetRegistry.h> 
#include <llvm/Analysis/TargetTransformInfo.h> 
#include <llvm/Analysis/TargetLibraryInfo.h> 

using namespace easy;

BitcodeTracker& BitcodeTracker::GetTracker() {
  static BitcodeTracker TheTracker;
  return TheTracker;
}

std::tuple<const char*, GlobalMapping*> BitcodeTracker::getNameAndGlobalMapping(void* FPtr) {
  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    throw std::runtime_error("Cannot name and global mapping. Function not registered");
  }

  return std::make_tuple(InfoPtr->second.Name, InfoPtr->second.Globals);
} 

llvm::Module* BitcodeTracker::getModule(void* FPtr) {

  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    throw std::runtime_error("Cannot find bitcode. Function not registered");
  }

  auto &Info = InfoPtr->second;

  // to avoid races when parsing the same bitcode
  std::lock_guard<std::mutex> Lock(*Info.CachedLock);

  if(Info.FJC) 
    return Info.FJC->Module.get();

  Info.FJC = std::make_shared<FunctionJitContext>(
      std::unique_ptr<llvm::LLVMContext>(new llvm::LLVMContext())); 

  llvm::StringRef BytecodeStr(Info.Bitcode, Info.BitcodeLen - 1);
  std::unique_ptr<llvm::MemoryBuffer> Buf(llvm::MemoryBuffer::getMemBuffer(BytecodeStr));
  auto ModuleOrErr =
      llvm::parseBitcodeFile(Buf->getMemBufferRef(), *Info.FJC->Context);

  if (auto EC = ModuleOrErr.takeError()) {
    throw std::runtime_error("Cannot parse bitcode.");
  }

  Info.FJC->Module = std::move(ModuleOrErr.get());
  return Info.FJC->Module.get();
} 

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

void Function::Optimize(llvm::Module& M, int OptLevel, int OptSize) {

  auto Triple = llvm::sys::getProcessTriple();

  llvm::PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;
  Builder.SizeLevel = OptSize;
  Builder.LibraryInfo = new llvm::TargetLibraryInfoImpl(llvm::Triple{Triple});

  std::unique_ptr<llvm::TargetMachine> TM = GetTargetMachine(Triple);

  llvm::legacy::PassManager MPM;
  MPM.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
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
    throw std::runtime_error("Failed to create ExecutionEngine.");
  }

  return EE;
}

void Function::MapGlobals(llvm::ExecutionEngine& EE, GlobalMapping* Globals) {
  for(GlobalMapping *GM = Globals; GM->Name; ++GM) {
    EE.addGlobalMapping(GM->Name, (uint64_t)GM->Address);
  }
}
