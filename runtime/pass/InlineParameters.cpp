#include <easy/runtime/RuntimePasses.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>
#include <numeric>

using namespace llvm;
using namespace easy;
using arg_ty = easy::Argument::Type;

char easy::InlineParameters::ID = 0;

llvm::Pass* easy::createInlineParametersPass(llvm::StringRef Name) {
  return new InlineParameters(Name);
}

FunctionType* GetWrapperTy(FunctionType *FTy, Context const &C) {

  Type* RetTy = FTy->getReturnType();

  size_t NewArgCount =
      std::accumulate(C.begin(), C.end(), 0, [](size_t Max, easy::Argument const &Arg) {
        return Arg.ty == arg_ty::Forward ?
          std::max<size_t>(Arg.data.param_idx+1, Max) : Max;
      });

  SmallVector<Type*, 8> Args(NewArgCount, nullptr);

  for(size_t i = 0, n = C.size(); i != n; ++i) {
    easy::Argument const &Arg = C.getArgumentMapping(i);
    if(Arg.ty != arg_ty::Forward)
      continue;
    if(!Args[Arg.data.param_idx])
      Args[Arg.data.param_idx] = FTy->getParamType(i);
  }

  return FunctionType::get(RetTy, Args, FTy->isVarArg());
}

void GetInlineArgs(Context const &C, FunctionType& OldTy, Function &Wrapper, SmallVectorImpl<Value*> &Args) {
  SmallVector<Value*, 8> WrapperArgs(Wrapper.getFunctionType()->getNumParams());
  std::transform(Wrapper.arg_begin(), Wrapper.arg_end(),
                 WrapperArgs.begin(), [](llvm::Argument &A)->Value*{return &A;});

  for(size_t i = 0, n = C.size(); i != n; ++i) {
    auto const &Arg = C.getArgumentMapping(i);
    switch (Arg.ty) {
      case arg_ty::Forward:
        Args.push_back(WrapperArgs[Arg.data.param_idx]);
        continue;
      case arg_ty::Int:
        Args.push_back(ConstantInt::get(OldTy.getParamType(i), Arg.data.integer, true));
        continue;
      case arg_ty::Float:
        Args.push_back(ConstantFP::get(OldTy.getParamType(i), Arg.data.floating));
        continue;
      case arg_ty::Ptr:
        Args.push_back(
              ConstantExpr::getIntToPtr(
                ConstantInt::get(Type::getInt64Ty(OldTy.getContext()), (uintptr_t)Arg.data.ptr, false),
                OldTy.getParamType(i)));
        continue;
    }
  }
}

Function* CreateWrapperFun(Module &M, FunctionType &WrapperTy, Function &F, Context const &C) {
  LLVMContext &CC = M.getContext();

  Function* Wrapper = Function::Create(&WrapperTy, Function::ExternalLinkage, "", &M);
  BasicBlock* BB = BasicBlock::Create(CC, "", Wrapper);

  SmallVector<Value*, 8> Args;
  GetInlineArgs(C, *F.getFunctionType(), *Wrapper, Args);

  IRBuilder<> B(BB);
  Value* Call = B.CreateCall(&F, Args);

  if(Call->getType()->isVoidTy()) {
    B.CreateRetVoid();
  } else {
    B.CreateRet(Call);
  }

  return Wrapper;
}

bool easy::InlineParameters::runOnModule(llvm::Module &M) {

  Context const &C = getAnalysis<ContextAnalysis>().getContext();
  Function* F = M.getFunction(TargetName_);
  assert(F);

  FunctionType* FTy = F->getFunctionType();
  assert(FTy->getNumParams() == C.size());

  FunctionType* WrapperTy = GetWrapperTy(FTy, C);
  Function* WrapperFun = CreateWrapperFun(M, *WrapperTy, *F, C);

  // privatize F and steal its name
  F->setLinkage(Function::PrivateLinkage);
  WrapperFun->takeName(F);

  return true;
}

static RegisterPass<easy::InlineParameters> X("","",false, false);
