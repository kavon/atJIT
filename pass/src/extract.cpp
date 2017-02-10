#include "extract.hpp"

#include "llvm/Pass.h"
#include "llvm/InitializePasses.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"

#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/Support/raw_ostream.h"

using namespace pass;
using namespace llvm;

namespace pass {

  // globals
  constexpr bool Debug = true;
  static const char* EnabledName = "easy_jit_enabled";
  static const char* EmbeddedMoudleName = "easy_jit_module";
  static const char* HookName = "easy_jit_hook";
  static const char* HookEndName = "easy_jit_hook_end";

  // pass configuration
  static const char* Command = "easy_jit";
  static const char* Description = "Pass to extract functions to ir.";
  constexpr bool IsAnalysis = false;
  constexpr bool IsCfgOnly = false;
  static RegisterPass<ExtractAndEmbed> X(Command, Description, IsCfgOnly, IsAnalysis);

  char ExtractAndEmbed::ID = 0;

  ExtractAndEmbed::ExtractAndEmbed() :
    llvm::ModulePass(ID) {
  }

  void ExtractAndEmbed::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
  }

  SmallPtrSet<Function*, 8> getFunctionsToExtract(Function* EasyJitEnabled) {
    assert(EasyJitEnabled);
    SmallPtrSet<Function*, 8> Fun2Extract;

    //collect the functions where the function is used
    while(!EasyJitEnabled->user_empty()) {
      User* U = EasyJitEnabled->user_back();
      CallInst* AsCall = dyn_cast<CallInst>(U);

      assert(AsCall && AsCall->getCalledFunction() == EasyJitEnabled
             && "Easy jit function not used as a function call!");

      Function* F = AsCall->getParent()->getParent();

      Fun2Extract.insert(F);
      AsCall->eraseFromParent();
    }

    if(Debug) {
      for(Function* f : Fun2Extract)
          errs() << "Function " << f->getName() << " marked for extraction.\n";
    }

    return Fun2Extract;
  }

  void AddJITHookDefinition(Module& M) {

    LLVMContext &C = M.getContext();

    Function* JitHook = M.getFunction(HookName);
    if(!JitHook) {
      Type* I8Ptr = Type::getInt8PtrTy(C);
      Type* I64 = Type::getInt64Ty(C);

      FunctionType* HookFnTy = FunctionType::get(I8Ptr, {I8Ptr, I8Ptr, I64}, false);

      JitHook = Function::Create(HookFnTy, Function::ExternalLinkage, HookName, &M);
    }

    Function* JitHookEnd = M.getFunction(HookEndName);
    if(!JitHookEnd) {
      Type* VoidTy = Type::getVoidTy(C);
      Type* I8Ptr = Type::getInt8PtrTy(C);
      FunctionType* HookEndFnTy = FunctionType::get(VoidTy, {I8Ptr}, false);

      JitHookEnd = Function::Create(HookEndFnTy, Function::ExternalLinkage, HookEndName, &M);
    }
  }

    bool addGlobalIfUsedByExtracted(GlobalValue& GV,
                                    const SmallPtrSetImpl<Function*> &Fun2Extract,
                                    SmallPtrSetImpl<GlobalValue*> &Globals) {
      for(User* U : GV.users()) {
        Instruction* UI = dyn_cast<Instruction>(U);
        if(!UI)
          continue;

        bool UsedInExtracted = Fun2Extract.count(UI->getParent()->getParent());
        if(!UsedInExtracted)
          continue;

        Globals.insert(&GV);
        return true;
      }

      return false;
    }

    template<class Range>
    void getReferencedGlobalsInRange(const SmallPtrSetImpl<Function*>& Fun2Extract,
                                    Range &&R,
                                    SmallPtrSetImpl<GlobalValue*> &Globals) {
      for(auto &FOrGV : R) {
        Function* F = dyn_cast<Function>(&FOrGV);
        if(Fun2Extract.count(F))
          continue;

        if(addGlobalIfUsedByExtracted(FOrGV, Fun2Extract, Globals) && Debug)
          errs() << "Global " << FOrGV.getName() << " referenced by extracted function.\n";
      }
    }

    SmallPtrSet<GlobalValue*, 8> getReferencedGlobals(const SmallPtrSetImpl<Function*>& Fun2Extract, Module& M) {
        SmallPtrSet<GlobalValue*, 8> Globals;
        getReferencedGlobalsInRange(Fun2Extract, M.globals(), Globals);
        getReferencedGlobalsInRange(Fun2Extract, M.functions(), Globals);
        return Globals;
    }

    bool ValidForExtraction(const SmallPtrSetImpl<GlobalValue*>& Globals) {
      auto Search = std::find_if(Globals.begin(), Globals.end(), [](GlobalValue const* GV) {
        return GV->hasPrivateLinkage() || GV->hasInternalLinkage();
      });

      if(Search != Globals.end() && Debug) {
        errs() << "Cannot extract module: global " << (*Search)->getName()
               << " has private/internal linkage.\n";
      }
      return Search == Globals.end();
    }

    void CreateJITHook(Function* F) {
        std::string FName = F->getName();

        Function* Hook = Function::Create(F->getFunctionType(), F->getLinkage(), "hook", F->getParent());
        F->replaceAllUsesWith(Hook);
        F->eraseFromParent();
        Hook->setName(FName);

        Module* M = Hook->getParent();
        LLVMContext& C = Hook->getContext();
        Type* I8Ptr = Type::getInt8PtrTy(C);
        
        //get the hook function to the runtime
        Function* JITHook = M->getFunction(HookName);
        Function* JITHookEnd = M->getFunction(HookEndName);
        assert(JITHook && JITHookEnd);
        
        //create a single block
        BasicBlock* Entry = BasicBlock::Create(C, "entry", Hook);
        IRBuilder<> B(Entry);
        
        //create a string
        Constant* Init = ConstantDataArray::getString(C, FName, true);
        Constant* FNameGV =
            new GlobalVariable(*M, Init->getType(), true, GlobalValue::InternalLinkage, Init, ".easy_jit_fun_name");
        FNameGV = ConstantExpr::getPointerCast(FNameGV, I8Ptr);
        
        //get the ir variable
        Constant* IRVar = M->getNamedGlobal(EmbeddedMoudleName);
        assert(IRVar);
        ArrayType* IRVarTy = cast<ArrayType>(IRVar->getType()->getContainedType(0));
        Constant* Size = ConstantInt::get(Type::getInt64Ty(C), IRVarTy->getNumElements());
        IRVar = ConstantExpr::getPointerCast(IRVar, I8Ptr);
        
        //create the call
        Value* HookArgs[] = {FNameGV, IRVar, Size};
        Value* JitResult = B.CreateCall(JITHook, HookArgs);
        Value* Cast = B.CreatePointerCast(JitResult, Hook->getType());

        SmallVector<std::reference_wrapper<Argument>, 8> FunArgs(Hook->arg_begin(), Hook->arg_end());
        SmallVector<Value*, 8> FunArgsVal(FunArgs.size());
        std::transform(FunArgs.begin(), FunArgs.end(), FunArgsVal.begin(), std::addressof<Value>);

        Value* Call = B.CreateCall(Cast, FunArgsVal);

        B.CreateCall(JITHookEnd, {JitResult});

        if(Call->getType()->isVoidTy())
            ReturnInst::Create(C, Entry);
        else
            ReturnInst::Create(C, Call, Entry);
    }

    void GVMakeExternalDeclaration(GlobalValue* GV) {
      GV->setLinkage(GlobalValue::ExternalLinkage);
      if(Function* F = dyn_cast<Function>(GV)) {
        F->getBasicBlockList().clear();
      } else if (GlobalVariable *G = dyn_cast<GlobalVariable>(GV)){
        G->setInitializer(nullptr);
      } else assert(false);
    }

    std::unique_ptr<Module> getModuleForJITCompilation(const SmallPtrSetImpl<Function*>& Fun2Extract, Module& M) {
        auto Globals = getReferencedGlobals(Fun2Extract, M);

        if(!ValidForExtraction(Globals))
          return nullptr;

        auto Clone = llvm::CloneModule(&M);
        auto GetGlobalInClone = [&Clone](auto* G) { return Clone->getNamedValue(G->getName()); };

        //collect the referenced globals in the clone
        std::vector<GlobalValue*> GlobalsToKeep(Globals.size() + Fun2Extract.size());
        std::transform(Globals.begin(), Globals.end(), GlobalsToKeep.begin(), GetGlobalInClone);
        std::transform(Fun2Extract.begin(), Fun2Extract.end(), GlobalsToKeep.begin() + Globals.size(), GetGlobalInClone);

        //clean the clonned module
        llvm::legacy::PassManager Passes;
        Passes.add(createGVExtractionPass(GlobalsToKeep));
        Passes.add(createGlobalDCEPass());
        Passes.add(createStripDeadDebugInfoPass());
        Passes.add(createStripDeadPrototypesPass());
        Passes.run(*Clone);

        //transform the globals to external declarations
        std::for_each(GlobalsToKeep.begin(), GlobalsToKeep.begin() + Globals.size(),
                      GVMakeExternalDeclaration);

        //rename the functions
        std::for_each(GlobalsToKeep.begin() + Globals.size(), GlobalsToKeep.end(),
        [](GlobalValue *GV) { GV->setName(GV->getName() + "__"); });

        return Clone;
    }

    std::string ModuleToString(Module &M) {
      std::string s;
      raw_string_ostream so(s);
      WriteBitcodeToFile(&M, so);
      so.flush();
      return s;
    }

    GlobalVariable* WriteModuleToGlobal(Module& M, Module& JitM) {
      std::string ModuleAsStr = ModuleToString(JitM);
      LLVMContext& C = M.getContext();

      Constant* Init = ConstantDataArray::getString(C, ModuleAsStr, true);
      GlobalVariable* BitcodeGV =
          new GlobalVariable(M, Init->getType(), true, GlobalValue::InternalLinkage, Init, EmbeddedMoudleName);

      if(Debug)
        errs() << "Extracted module written to " << BitcodeGV->getName() << "\n";

      return BitcodeGV;
    }

    bool ExtractAndEmbed::runOnModule(llvm::Module& M) {
      Function* EasyJitEnabled = M.getFunction(EnabledName);

      if(M.getNamedGlobal(EmbeddedMoudleName)) {
        errs() << "WARNING: Compilation unit already contains an extracted module.\n";
        return false;
      }

      if(!EasyJitEnabled)
        return false;

      auto Fun2Extract = getFunctionsToExtract(EasyJitEnabled);

      if(Fun2Extract.empty())
        return false;

      // TODO: This extracts all the functions in the same module,
      //    extract each function in a separated module!
      auto JitM = getModuleForJITCompilation(Fun2Extract, M);

      if(!JitM)
        return false;

      WriteModuleToGlobal(M, *JitM);

      //drop extracted functions
      AddJITHookDefinition(M);
      for(Function* F : Fun2Extract) {
        CreateJITHook(F);
      }

      return true;
    }
}
