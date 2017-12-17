#include <easy/runtime/BitcodeTracker.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/raw_ostream.h>

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

std::tuple<const char*, GlobalMapping*> BitcodeTracker::getNameAndGlobalMapping(void* FPtr) {
  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    throw easy::BitcodeNotRegistered();
  }

  return std::make_tuple(InfoPtr->second.Name, InfoPtr->second.Globals);
}

llvm::Module* BitcodeTracker::getModule(void* FPtr) {

  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    throw easy::BitcodeNotRegistered();
  }

  auto &Info = InfoPtr->second;

  // to avoid races when parsing the same bitcode
  std::lock_guard<std::mutex> Lock(*Info.CachedLock);

  if(Info.FJC)
    return Info.FJC->Module.get();

  Info.FJC = std::make_shared<FunctionJitContext>(
      std::unique_ptr<llvm::LLVMContext>(new llvm::LLVMContext()));

  llvm::StringRef BytecodeStr(Info.Bitcode, Info.BitcodeLen);
  std::unique_ptr<llvm::MemoryBuffer> Buf(llvm::MemoryBuffer::getMemBuffer(BytecodeStr));
  auto ModuleOrErr =
      llvm::parseBitcodeFile(Buf->getMemBufferRef(), *Info.FJC->Context);

  if (llvm::Error EC = ModuleOrErr.takeError()) {
    throw easy::BitcodeParseError(Info.Name);
  }

  Info.FJC->Module = std::move(ModuleOrErr.get());
  return Info.FJC->Module.get();
}

// function to interface with the generated code
void easy_register(void* FPtr, const char* Name, GlobalMapping* Globals, const char* Bitcode, size_t BitcodeLen) {
  BitcodeTracker::GetTracker().registerFunction(FPtr, Name, Globals, Bitcode, BitcodeLen);
}


