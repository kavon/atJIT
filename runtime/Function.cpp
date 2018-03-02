#include <easy/runtime/BitcodeTracker.h>
#include <easy/runtime/Function.h>
#include <easy/runtime/RuntimePasses.h>
#include <easy/runtime/LLVMHolderImpl.h>
#include <easy/runtime/Utils.h>
#include <easy/exceptions.h>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>
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

static std::unique_ptr<llvm::TargetMachine> GetHostTargetMachine() {
  std::unique_ptr<llvm::TargetMachine> TM(llvm::EngineBuilder().selectTarget());
  return TM;
}

static void Optimize(llvm::Module& M, const char* Name, const easy::Context& C, unsigned OptLevel, unsigned OptSize) {

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

static std::unique_ptr<llvm::ExecutionEngine> GetEngine(std::unique_ptr<llvm::Module> M, const char *Name) {
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

static void MapGlobals(llvm::ExecutionEngine& EE, GlobalMapping* Globals) {
  for(GlobalMapping *GM = Globals; GM->Name; ++GM) {
    EE.addGlobalMapping(GM->Name, (uint64_t)GM->Address);
  }
}

static void WriteOptimizedToFile(llvm::Module const &M, std::string const& File) {
  if(File.empty())
    return;
  std::error_code Error;
  llvm::raw_fd_ostream Out(File, Error, llvm::sys::fs::F_None);

  if(Error)
    throw CouldNotOpenFile(Error.message());

  Out << M;
}

std::unique_ptr<Function>
CompileAndWrap(const char*Name, GlobalMapping* Globals,
               std::unique_ptr<llvm::LLVMContext> Ctx,
               std::unique_ptr<llvm::Module> M) {

  llvm::Module* MPtr = M.get();
  std::unique_ptr<llvm::ExecutionEngine> EE = GetEngine(std::move(M), Name);

  if(Globals) {
    MapGlobals(*EE, Globals);
  }

  void *Address = (void*)EE->getFunctionAddress(Name);

  std::unique_ptr<LLVMHolder> Holder(new easy::LLVMHolderImpl{std::move(EE), std::move(Ctx), MPtr});
  return std::unique_ptr<Function>(new Function(Address, std::move(Holder)));
}

llvm::Module const& Function::getLLVMModule() const {
  return *static_cast<LLVMHolderImpl const&>(*this->Holder).M_;
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

  return CompileAndWrap(Name, Globals, std::move(Ctx), std::move(M));
}

void easy::Function::serialize(std::ostream& os) const {
  std::string buf;
  llvm::raw_string_ostream stream(buf);

  LLVMHolderImpl const *H = reinterpret_cast<LLVMHolderImpl const*>(Holder.get());
  llvm::WriteBitcodeToFile(H->M_, stream);
  stream.flush();

  os << buf;
}

std::unique_ptr<easy::Function> easy::Function::deserialize(std::istream& is) {

  auto &BT = BitcodeTracker::GetTracker();

  std::string buf(std::istreambuf_iterator<char>(is), {}); // read the entire istream
  auto MemBuf = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(buf));

  std::unique_ptr<llvm::LLVMContext> Ctx(new llvm::LLVMContext());
  auto ModuleOrError = llvm::parseBitcodeFile(*MemBuf, *Ctx);
  if(ModuleOrError.takeError()) {
    return nullptr;
  }

  auto M = std::move(ModuleOrError.get());

  std::string FunName = easy::GetEntryFunctionName(*M);

  GlobalMapping* Globals = nullptr;
  if(void* OrigFunPtr = BT.getAddress(FunName)) {
    std::tie(std::ignore, Globals) = BT.getNameAndGlobalMapping(OrigFunPtr);
  }

  return CompileAndWrap(FunName.c_str(), Globals, std::move(Ctx), std::move(M));
}

bool Function::operator==(easy::Function const& other) const {
  LLVMHolderImpl& This = static_cast<LLVMHolderImpl&>(*this->Holder);
  LLVMHolderImpl& Other = static_cast<LLVMHolderImpl&>(*other.Holder);
  return This.M_ == Other.M_;
}

std::hash<easy::Function>::result_type
std::hash<easy::Function>::operator()(argument_type const& F) const noexcept {
  LLVMHolderImpl& This = static_cast<LLVMHolderImpl&>(*F.Holder);
  return std::hash<llvm::Module*>{}(This.M_);
}
