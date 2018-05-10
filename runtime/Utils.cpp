#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallVector.h>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Bitcode/BitcodeReader.h>

#include <string>

#include <easy/runtime/Utils.h>

using namespace llvm;

static const char EasyJitMD[] = "easy::jit";
static const char EntryTag[] = "entry";

std::string easy::GetEntryFunctionName(Module const &M) {
  NamedMDNode* MD = M.getNamedMetadata(EasyJitMD);

  for(MDNode *Operand : MD->operands()) {
    if(Operand->getNumOperands() != 2)
      continue;
    MDString* Entry = dyn_cast<MDString>(Operand->getOperand(0));
    MDString* Name = dyn_cast<MDString>(Operand->getOperand(1));

    if(!Entry || !Name || Entry->getString() != EntryTag)
      continue;

    return Name->getString();
  }

  llvm_unreachable("No entry function in easy::jit module!");
  return "";
}

void easy::MarkAsEntry(llvm::Function &F) {
  Module &M = *F.getParent();
  LLVMContext &Ctx = F.getContext();
  NamedMDNode* MD = M.getOrInsertNamedMetadata(EasyJitMD);
  MDNode* Node = MDNode::get(Ctx, { MDString::get(Ctx, EntryTag),
                                    MDString::get(Ctx, F.getName())});
  MD->addOperand(Node);
}

void easy::UnmarkEntry(llvm::Module &M) {
  NamedMDNode* MD = M.getOrInsertNamedMetadata(EasyJitMD);
  M.eraseNamedMetadata(MD);
}

std::unique_ptr<llvm::Module>
easy::CloneModuleWithContext(llvm::Module const &LM, llvm::LLVMContext &C) {
  // I have not found a better way to do this withouth having to fully reimplement
  // CloneModule

  std::string buf;

  // write module
  {
    llvm::raw_string_ostream stream(buf);
    llvm::WriteBitcodeToFile(&LM, stream);
    stream.flush();
  }

  // read the module
  auto MemBuf = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(buf));
  auto ModuleOrError = llvm::parseBitcodeFile(*MemBuf, C);
  if(ModuleOrError.takeError())
    return nullptr;

  auto LMCopy = std::move(ModuleOrError.get());
  return std::move(LMCopy);
}
