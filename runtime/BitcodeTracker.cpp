#include <easy/runtime/BitcodeTracker.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Support/raw_ostream.h>

using namespace easy;
using namespace llvm;

BitcodeTracker& BitcodeTracker::GetTracker() {
  static BitcodeTracker TheTracker;
  return TheTracker;
}

std::tuple<const char*, GlobalMapping*> BitcodeTracker::getNameAndGlobalMapping(void* FPtr) {
  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    // TODO throw easy::exception
    throw std::runtime_error("Cannot find name and global mapping. Function not registered");
  }

  return std::make_tuple(InfoPtr->second.Name, InfoPtr->second.Globals);
}

llvm::Module* BitcodeTracker::getModule(void* FPtr) {

  auto InfoPtr = Functions.find(FPtr);
  if(InfoPtr == Functions.end()) {
    // TODO throw easy::exception
    throw std::runtime_error("Cannot find bitcode. Function not registered");
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

  if (auto EC = ModuleOrErr.takeError()) {
    // TODO throw easy::exception
    throw std::runtime_error("Cannot parse bitcode.");
  }

  Info.FJC->Module = std::move(ModuleOrErr.get());
  return Info.FJC->Module.get();
}

// function to interface with the generated code
void easy_register(void* FPtr, const char* Name, GlobalMapping* Globals, const char* Bitcode, size_t BitcodeLen) {
  BitcodeTracker::GetTracker().registerFunction(FPtr, Name, Globals, Bitcode, BitcodeLen);
}


