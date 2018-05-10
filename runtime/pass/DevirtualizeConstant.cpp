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
    auto* Location = dyn_cast<Constant>(VTable->getPointerOperand()->stripPointerCasts());
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
    auto &BT = easy::BitcodeTracker::GetTracker();
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

bool easy::DevirtualizeConstant::runOnFunction(llvm::Function &F) {

  if(F.getName() != TargetName_)
    return false;

  llvm::Module &M = *F.getParent();

  easy::Context const &C = getAnalysis<ContextAnalysis>().getContext();

  for(auto& I: instructions(F)) {
    auto* VTable = getVTableHostAddress(I);
    if(!VTable)
      continue;

    void** RuntimeLoadedValue = *(void***)(uintptr_t)(VTable->getZExtValue());

    // that's generally the load from the table
    for(User* U : VTable->users()) {
      ConstantExpr* CE = dyn_cast<ConstantExpr>(U);
      if(!CE || !CE->isCast())
        continue;

      for(User* U : CE->users()) {
        if(auto* CalledPtr = dyn_cast<LoadInst>(U)) {
          void* CalledPtrHostValue = *RuntimeLoadedValue;
          llvm::Function* Called = findFunctionAndLinkModules(M, CalledPtrHostValue);
          if(Called) {
            for(User* U2 : CalledPtr->users()) {
              if(auto* LI = dyn_cast<LoadInst>(U2))
                LI->replaceAllUsesWith(Called);
            }
          }
        }
      }
    }
  }

  return true;
}

static RegisterPass<easy::InlineParameters> X("","",false, false);
