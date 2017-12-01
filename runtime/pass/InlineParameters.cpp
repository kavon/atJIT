#include <easy/runtime/RuntimePasses.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace easy;
using arg_ty = easy::Argument::Type;

char easy::InlineParameters::ID = 0;

llvm::Pass* easy::createInlineParametersPass(llvm::StringRef Name) {
  return new InlineParameters(Name);
}

FunctionType* GetWrapperTy(FunctionType *FTy, Context const &C, DenseMap<size_t, size_t> &ParamToNewPosition) {

  Type* RetTy = FTy->getReturnType();
  SmallVector<Type*, 8> Args;

  // since we expect a small number of arguments, perefer a small vector over of a set
  SmallVector<size_t, 8> ArgsToForward;
  for(auto const &Arg : C)
    if(Arg.ty == arg_ty::Forward)
      ArgsToForward.push_back(Arg.data.param_idx);

  // sort and remeove duplicates
  std::sort(ArgsToForward.begin(), ArgsToForward.end());
  ArgsToForward.erase(std::unique(ArgsToForward.begin(), ArgsToForward.end()), ArgsToForward.end());

  size_t new_position = 0;
  for(size_t arg_idx : ArgsToForward) {
    ParamToNewPosition[arg_idx] = new_position;
    Args.push_back(FTy->getParamType(arg_idx));
  }

  return FunctionType::get(RetTy, Args, FTy->isVarArg());
}

void GetInlineArgs(Context const &C, FunctionType& OldTy, Function &Wrapper, SmallVectorImpl<Value*> &Args, DenseMap<size_t, size_t> &ParamNewPos) {
  SmallVector<Value*, 8> WrapperArgs;
  for(Value &Arg : Wrapper.args())
    WrapperArgs.push_back(&Arg);

  for(size_t arg_idx = 0, n = C.size(); arg_idx != n; ++arg_idx) {
    auto const &Arg = C.getArgumentMapping(arg_idx);
    switch (Arg.ty) {
      case arg_ty::Forward:
        Args.push_back(WrapperArgs[ParamNewPos[Arg.data.param_idx]]);
        continue;
      case arg_ty::Int:
        Args.push_back(ConstantInt::get(OldTy.getParamType(arg_idx), Arg.data.integer, true));
        continue;
      case arg_ty::Float:
        Args.push_back(ConstantFP::get(OldTy.getParamType(arg_idx), Arg.data.floating));
        continue;
      case arg_ty::Ptr:
        Args.push_back(
              ConstantExpr::getPtrToInt(
                ConstantInt::get(Type::getInt64Ty(OldTy.getContext()), (uintptr_t)Arg.data.ptr, false),
                OldTy.getParamType(arg_idx)));
        continue;
    }
  }
}

Function* CreateWrapperFun(Module &M, FunctionType &WrapperTy, Function &F, Context const &C, DenseMap<size_t, size_t> &ParamNewPos) {
  LLVMContext &CC = M.getContext();

  Function* Wrapper = Function::Create(&WrapperTy, Function::ExternalLinkage, "", &M);
  BasicBlock* BB = BasicBlock::Create(CC, "", Wrapper);

  SmallVector<Value*, 8> Args;
  GetInlineArgs(C, *F.getFunctionType(), *Wrapper, Args, ParamNewPos);

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

  DenseMap<size_t, size_t> ParamNewPos;
  FunctionType* WrapperTy = GetWrapperTy(FTy, C, ParamNewPos);
  Function* WrapperFun = CreateWrapperFun(M, *WrapperTy, *F, C, ParamNewPos);

  // privatize F and steal its name
  F->setLinkage(Function::PrivateLinkage);
  WrapperFun->takeName(F);

  return true;
}

static RegisterPass<easy::InlineParameters> X("","",false, false);
