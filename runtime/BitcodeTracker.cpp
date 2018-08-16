#include <easy/runtime/BitcodeTracker.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/DiagnosticHandler.h>

#include <easy/exceptions.h>

using namespace easy;
using namespace llvm;

namespace easy {
  DefineEasyException(BitcodeNotRegistered, "Cannot find bitcode.");
  DefineEasyException(BitcodeParseError, "Cannot parse bitcode for: ");
}

BitcodeTracker& BitcodeTracker::GetTracker() {
  static BitcodeTracker TheTracker;
  return TheTracker;
}

bool BitcodeTracker::hasGlobalMapping(void* FPtr) const {
  auto InfoPtr = Functions.find(FPtr);
  return InfoPtr != Functions.end();
}

void* BitcodeTracker::getAddress(std::string const &Name) {
  auto Addr = NameToAddress.find(Name);
  if(Addr == NameToAddress.end())
    return nullptr;
  return Addr->second;
}

std::tuple<const char*, GlobalMapping*> BitcodeTracker::getNameAndGlobalMapping(void* FPtr) {
  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    throw easy::BitcodeNotRegistered();
  }

  return std::make_tuple(InfoPtr->second.Name, InfoPtr->second.Globals);
}

const char* BitcodeTracker::getName(void* FPtr) {
  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    throw easy::BitcodeNotRegistered();
  }

  return InfoPtr->second.Name;
}

std::unique_ptr<llvm::Module> BitcodeTracker::getModuleWithContext(void* FPtr, llvm::LLVMContext &C) {
  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    throw easy::BitcodeNotRegistered();
  }

  auto &Info = InfoPtr->second;

  llvm::StringRef BytecodeStr(Info.Bitcode, Info.BitcodeLen);
  std::unique_ptr<llvm::MemoryBuffer> Buf(llvm::MemoryBuffer::getMemBuffer(BytecodeStr));
  auto ModuleOrErr =
      llvm::parseBitcodeFile(Buf->getMemBufferRef(), C);

  if (ModuleOrErr.takeError()) {
    throw easy::BitcodeParseError(Info.Name);
  }

  return std::move(ModuleOrErr.get());
}

#ifdef NDEBUG
class DiagnosticSilencer : public llvm::DiagnosticHandler {
public:
  DiagnosticSilencer() {}
  bool handleDiagnostics(const DiagnosticInfo &DI) override { return true; }
  bool isAnalysisRemarkEnabled(StringRef PassName) const override { return false; }
  bool isMissedOptRemarkEnabled(StringRef PassName) const override { return false; }
  bool isPassedOptRemarkEnabled(StringRef PassName) const override { return false; }
}; // end class
#endif

BitcodeTracker::ModuleContextPair BitcodeTracker::getModule(void* FPtr) {

  std::unique_ptr<llvm::LLVMContext> Context(new llvm::LLVMContext());

#ifdef NDEBUG
  // silence the output as much as possible!
  Context->setDiagnosticHandler(std::make_unique<DiagnosticSilencer>());
  Context->setDiagnosticsHotnessThreshold(~0);
#endif

  auto Module = getModuleWithContext(FPtr, *Context);

  return ModuleContextPair(std::move(Module), std::move(Context));
}

// function to interface with the generated code
extern "C" {
void easy_register(void* FPtr, const char* Name, GlobalMapping* Globals, const char* Bitcode, size_t BitcodeLen) {
  BitcodeTracker::GetTracker().registerFunction(FPtr, Name, Globals, Bitcode, BitcodeLen);
}
}
