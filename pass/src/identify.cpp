#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include "identify.h"

using namespace llvm;

static const char* EnabledName = "easy_jit_enabled";
static const char* SpecializePrefix = "easy_jit_specialize";

static void GetInlineParameters(Module &M, FunToInlineMap &Map) {
  for(Function &F : M) {
    if(!F.getName().startswith(SpecializePrefix))
      continue;

    SmallVector<User*, 8> Users(F.user_begin(), F.user_end());

    for(User* U : Users) {
      CallInst* AsCall = dyn_cast<CallInst>(U);
      assert(AsCall && AsCall->getCalledFunction() == &F
             && "Easy jit function not used as a function call!");

      Function *ParentFun = AsCall->getParent()->getParent();
      Value* ToInline = AsCall->getArgOperand(0);

      assert(Map.count(ParentFun));
      Map[ParentFun].push_back(ToInline);

      AsCall->eraseFromParent();
    }
  }
}

FunToInlineMap GetFunctionsToJit(llvm::Module &M) {
  FunToInlineMap Map;

  // nothing marked
  Function* EasyJitEnabled = M.getFunction(EnabledName);
  if(!EasyJitEnabled)
    return Map;

  //collect the functions where the function is used
  while(!EasyJitEnabled->user_empty()) {
    User* U = EasyJitEnabled->user_back();
    CallInst* AsCall = dyn_cast<CallInst>(U);

    assert(AsCall && AsCall->getCalledFunction() == EasyJitEnabled
           && "Easy jit function not used as a function call!");

    Values Args;
    for (auto &Arg : AsCall->arg_operands())
      Args.push_back(Arg.get());

    Function* F = AsCall->getParent()->getParent();
    Map[F] = Args;

    AsCall->eraseFromParent();
  }

  GetInlineParameters(M, Map);

  return Map;
}

llvm::SmallVector<llvm::Function *, 8> GetFunctions(FunToInlineMap const &Map) {
  llvm::SmallVector<llvm::Function *, 8> Ret(Map.size());
  size_t idx = 0;
  for (auto const &pair : Map) {
    Ret[idx++] = pair.first;
  }
  return Ret;
}
