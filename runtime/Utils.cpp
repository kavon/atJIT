#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallVector.h>
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
