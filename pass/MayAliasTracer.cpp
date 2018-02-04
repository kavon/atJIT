#include "MayAliasTracer.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IntrinsicInst.h>

using namespace llvm;

void easy::MayAliasTracer::mayAliasWithStoredValues(Value* V, VSet &Loaded, VSet &Stored) {
  if(!Stored.insert(V).second)
    return;
  if(auto* GO = dyn_cast<GlobalObject>(V))
    GOs_.insert(GO);

  if(auto * II = dyn_cast<IntrinsicInst>(V)) {
    if(II->getIntrinsicID() == Intrinsic::memcpy) {
      mayAliasWithLoadedValues(II->getArgOperand(1), Loaded, Stored);
    }
  }

  if(auto* SI = dyn_cast<StoreInst>(V)) {
    mayAliasWithLoadedValues(SI->getValueOperand(), Loaded, Stored);
  }

  if(isa<AllocaInst>(V)||isa<GetElementPtrInst>(V)||isa<BitCastInst>(V)) {
    for(User* U : V->users()) {
      mayAliasWithStoredValues(U, Loaded, Stored);
    }
  }
}

void easy::MayAliasTracer::mayAliasWithLoadedValues(Value * V, VSet &Loaded, VSet &Stored) {
  if(!Loaded.insert(V).second)
    return;
  if(auto* GO = dyn_cast<GlobalObject>(V))
    GOs_.insert(GO);

  auto mayAliasWithLoadedOperand = [this, &Loaded, &Stored](Value* V) { mayAliasWithLoadedValues(V, Loaded, Stored);};

  //TODO: generalize that
  if(auto* PHI = dyn_cast<PHINode>(V)) {
    std::for_each(PHI->op_begin(), PHI->op_end(), mayAliasWithLoadedOperand);
  }
  if(auto* Select = dyn_cast<SelectInst>(V)) {
    mayAliasWithLoadedValues(Select->getTrueValue(), Loaded, Stored);
    mayAliasWithLoadedValues(Select->getFalseValue(), Loaded, Stored);
  }
  if(auto* Alloca = dyn_cast<AllocaInst>(V)) {
    mayAliasWithStoredValues(Alloca, Loaded, Stored);
  }
  if(auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
    mayAliasWithLoadedValues(GEP->getPointerOperand(), Loaded, Stored);
  }
  if(auto *BC = dyn_cast<BitCastInst>(V)) {
    mayAliasWithLoadedValues(BC->getOperand(0), Loaded, Stored);
  }
  if(auto const* CE = dyn_cast<ConstantExpr const>(V)) {
    switch(CE->getOpcode()) {
      case Instruction::GetElementPtr:
      case Instruction::BitCast:
        return mayAliasWithLoadedValues(CE->getOperand(0), Loaded, Stored);
      default:
        ;
    }
  }
  if(auto* OtherGV = dyn_cast<GlobalVariable>(V)) {
    if(OtherGV->hasInitializer())
      mayAliasWithLoadedValues(OtherGV->getInitializer(), Loaded, Stored);
    mayAliasWithStoredValues(OtherGV, Loaded, Stored);
  }
  if(auto* CA = dyn_cast<ConstantArray>(V)) {
    std::for_each(CA->op_begin(), CA->op_end(), mayAliasWithLoadedOperand);
  }
}
