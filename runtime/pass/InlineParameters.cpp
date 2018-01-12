#include <easy/runtime/RuntimePasses.h>
#include <easy/runtime/BitcodeTracker.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Linker/Linker.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Support/raw_ostream.h>
#include <numeric>

using namespace llvm;
using namespace easy;

char easy::InlineParameters::ID = 0;

llvm::Pass* easy::createInlineParametersPass(llvm::StringRef Name) {
  return new InlineParameters(Name);
}

static size_t GetNewArgCount(easy::Context const &C) {
  size_t Max = 0;
  for(auto const &Arg : C) {
    if(auto const* Forward = Arg->as<easy::ForwardArgument>()) {
      Max = std::max<size_t>(Forward->get()+1, Max);
    }
  }
  return Max;
}

FunctionType* GetWrapperTy(FunctionType *FTy, Context const &C) {

  Type* RetTy = FTy->getReturnType();

  size_t NewArgCount = GetNewArgCount(C);
  SmallVector<Type*, 8> Args(NewArgCount, nullptr);

  for(size_t i = 0, n = C.size(); i != n; ++i) {
    if(auto const *Arg = C.getArgumentMapping(i).as<easy::ForwardArgument>()) {
      size_t param_idx = Arg->get();
      if(!Args[param_idx])
        Args[param_idx] = FTy->getParamType(i);
    }
  }

  return FunctionType::get(RetTy, Args, FTy->isVarArg());
}

void GetInlineArgs(Context const &C, FunctionType& OldTy, Function &Wrapper, SmallVectorImpl<Value*> &Args, IRBuilder<> &B) {
  LLVMContext &Ctx = OldTy.getContext();
  SmallVector<Value*, 8> WrapperArgs(Wrapper.getFunctionType()->getNumParams());
  std::transform(Wrapper.arg_begin(), Wrapper.arg_end(),
                 WrapperArgs.begin(), [](llvm::Argument &A)->Value*{return &A;});

  for(size_t i = 0, n = C.size(); i != n; ++i) {
    auto const &Arg = C.getArgumentMapping(i);
    Type* ParamTy = OldTy.getParamType(i);
    if(auto const *Forward = Arg.as<ForwardArgument>()) {
      Args.push_back(WrapperArgs[Forward->get()]);
    } else if(auto const *Int = Arg.as<IntArgument>()) {
      Args.push_back(ConstantInt::get(ParamTy, Int->get(), true));
    } else if(auto const *Float = Arg.as<FloatArgument>()) {
      Args.push_back(ConstantFP::get(ParamTy, Float->get()));
    } else if(auto const *Ptr = Arg.as<PtrArgument>()) {
      Value* Repl = nullptr;
      auto &BT = BitcodeTracker::GetTracker();
      void* PtrValue = const_cast<void*>(Ptr->get());
      if(BT.hasGlobalMapping(PtrValue)) {
        const char* LName = std::get<0>(BT.getNameAndGlobalMapping(PtrValue));
        std::unique_ptr<Module> LM = BT.getModuleWithContext(PtrValue, Ctx);
        Module* M = Wrapper.getParent();

        if(!Linker::linkModules(*M, std::move(LM), Linker::OverrideFromSrc,
                                [](Module &, const StringSet<> &){}))
        {
          GlobalValue *GV = M->getNamedValue(LName);
          if(GlobalVariable* G = dyn_cast<GlobalVariable>(GV)) {
            GV->setLinkage(Function::PrivateLinkage);
            Repl = GV;
          }
          else if(Function* F = dyn_cast<Function>(GV)) {
            F->setLinkage(Function::PrivateLinkage);
            Repl = F;
          }
          else {
            assert(false && "wtf");
          }
        }
      }
      if(!Repl) { // default
        Repl =
            ConstantExpr::getIntToPtr(
              ConstantInt::get(Type::getInt64Ty(Ctx), (uintptr_t)Ptr->get(), false),
              ParamTy);
      }

      Args.push_back(Repl);

    } else if(auto const *Struct = Arg.as<StructArgument>()) {
      Type* Int8 = Type::getInt8Ty(Ctx);
      std::vector<char> const &Raw =  Struct->get();
      std::vector<Constant*> Data(Raw.size());
      for(size_t i = 0, n = Raw.size(); i != n; ++i)
        Data[i] = ConstantInt::get(Int8, Raw[i], false);
      Constant* CD = ConstantVector::get(Data);

      bool PassedByPtr = ParamTy->isPointerTy();
      Type* StructType = PassedByPtr ?
                  ParamTy->getContainedType(0) : ParamTy;

      if(ParamTy->isPointerTy()) {
        // big structures are passed by pointer, create a local alloca
        AllocaInst* Alloc = B.CreateAlloca(StructType, 0, "param_alloc");
        Value* AllocCast =
                B.CreatePointerCast(Alloc,
                                    PointerType::getUnqual(CD->getType()));
        B.CreateStore(CD, AllocCast);
        Args.push_back(Alloc);
      } else {
        // small structures are passed by value, cast it directly
        Constant* ConstantStruct = ConstantExpr::getBitCast(CD, StructType);
        Args.push_back(ConstantStruct);
      }
    }
  }
}

Function* CreateWrapperFun(Module &M, FunctionType &WrapperTy, Function &F, Context const &C) {
  LLVMContext &CC = M.getContext();

  Function* Wrapper = Function::Create(&WrapperTy, Function::ExternalLinkage, "", &M);
  BasicBlock* BB = BasicBlock::Create(CC, "", Wrapper);
  IRBuilder<> B(BB);

  SmallVector<Value*, 8> Args;
  GetInlineArgs(C, *F.getFunctionType(), *Wrapper, Args, B);

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

  // privatize F, steal its name, copy its attributes, and its cc
  F->setLinkage(Function::PrivateLinkage);
  WrapperFun->takeName(F);
  WrapperFun->setCallingConv(F->getCallingConv());

  auto FunAttrs = F->getAttributes().getFnAttributes();
  for(Attribute Attr : FunAttrs)
    WrapperFun->addFnAttr(Attr);

  return true;
}

static RegisterPass<easy::InlineParameters> X("","",false, false);
