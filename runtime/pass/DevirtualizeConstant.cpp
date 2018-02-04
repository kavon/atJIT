#include <easy/runtime/RuntimePasses.h>
#include <easy/runtime/BitcodeTracker.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Linker/Linker.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/raw_ostream.h>
#include <numeric>

using namespace llvm;
using namespace easy;

char easy::DevirtualizeConstant::ID = 0;

llvm::Pass* easy::createDevirtualizeConstantPass(llvm::StringRef Name) {
  return new DevirtualizeConstant(Name);
}

static ConstantInt* getVTableHostAddress(Value& V) {
    auto* VTable = dyn_cast<LoadInst>(&V);
    if(!VTable)
      return nullptr;
    MDNode *Tag = VTable->getMetadata(LLVMContext::MD_tbaa);
    if(!Tag || !Tag->isTBAAVtableAccess())
      return nullptr;

    // that's a vtable
    auto* Location = dyn_cast<Constant>(VTable->getPointerOperand());
    if(!Location)
      return nullptr;

    if(auto* CE = dyn_cast<ConstantExpr>(Location)) {
      if(CE->getOpcode() == Instruction::IntToPtr) {
        Location = CE->getOperand(0);
      }
    }
    auto* CLocation = dyn_cast<ConstantInt>(Location);
    if(!CLocation)
      return nullptr;
    return CLocation;
}

static Function* findFunctionAndLinkModules(Module& M, void* HostValue) {
    auto &BT = BitcodeTracker::GetTracker();
    const char* FName = std::get<0>(BT.getNameAndGlobalMapping(HostValue));

    if(!FName)
      return nullptr;

    std::unique_ptr<Module> LM = BT.getModuleWithContext(HostValue, M.getContext());

    if(!Linker::linkModules(M, std::move(LM), Linker::OverrideFromSrc,
                            [](Module &, const StringSet<> &){}))
    {
      GlobalValue *GV = M.getNamedValue(FName);
      if(Function* F = dyn_cast<Function>(GV)) {
        F->setLinkage(Function::PrivateLinkage);
        return F;
      }
      else {
        assert(false && "wtf");
      }
    }
    return nullptr;
}

bool easy::DevirtualizeConstant::runOnModule(llvm::Module &M) {

  Context const &C = getAnalysis<ContextAnalysis>().getContext();
  Function* F = M.getFunction(TargetName_);
  assert(F);

  for(auto& I: instructions(*F)) {
    auto* VTable = getVTableHostAddress(I);
    if(!VTable)
      continue;

    auto* VTableTy = VTable->getType();

    void** RuntimeLoadedValue = *(void***)(uintptr_t)(VTable->getZExtValue());

    // that's generally the load from the table
    for(User* U : VTable->users()) {
      if(auto* FPtr = dyn_cast<LoadInst>(U)) {
        void* FPtrHostValue = *RuntimeLoadedValue;
        Function* F = findFunctionAndLinkModules(M, FPtrHostValue);
        if(F) {
          FPtr->replaceAllUsesWith(F);
        }

      }
    }
  }

  return true;
}

static RegisterPass<easy::InlineParameters> X("","",false, false);
