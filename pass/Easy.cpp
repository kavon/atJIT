#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>

#include <llvm/IR/LegacyPassManager.h>

#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <llvm/Transforms/Utils/CtorUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <llvm/Bitcode/BitcodeWriter.h>

#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace easy {
  struct RegisterBitcode : public ModulePass {
    static char ID;

    RegisterBitcode()
      : ModulePass(ID) { };

    bool runOnModule(Module &M) override {

      SmallVector<Function*, 8> FunsToJIT;
      collectFunctionsToJIT(M, FunsToJIT);

      if(FunsToJIT.empty())
        return false;

      SmallVector<GlobalValue*, 8> LocalVariables;
      collectLocalGlobals(M, LocalVariables);
      nameGlobals(LocalVariables, "unnamed_local_global");

      auto Bitcode = embedBitcode(M, FunsToJIT);
      GlobalVariable* GlobalMapping = getGlobalMapping(M, LocalVariables);

      Function* RegisterBitcodeFun = declareRegisterBitcode(M, GlobalMapping);
      registerBitcode(M, FunsToJIT, Bitcode, GlobalMapping, RegisterBitcodeFun);

      return true;
    }

    private:

    static bool canExtractBitcode(Function &F, std::string &Reason) {
      if(F.isDeclaration()) {
        Reason = "Can't extract a declaration.";
        return false;
      }
      return true;
    }

    static void collectFunctionsToJIT(Module &M, SmallVectorImpl<Function*> &FunsToJIT) {
      for(Function &F : M) {
        if(F.getSection() != "jit")
          continue;

        std::string Reason;
        if(!canExtractBitcode(F, Reason)) {
          M.getContext().emitError(F.getName() + ": " + Reason);
          continue;
        }

        FunsToJIT.push_back(&F);
      }
    }

    static void collectLocalGlobals(Module &M, SmallVectorImpl<GlobalValue*> &Globals) {
      for(GlobalVariable &GV : M.globals())
        if(GV.hasLocalLinkage())
          Globals.push_back(&GV);
    }

    static void nameGlobals(SmallVectorImpl<GlobalValue*> &Globals, Twine Name) {
      for(GlobalValue *GV : Globals)
        if(!GV->hasName())
          GV->setName(Name);
    }

    static GlobalVariable*
    getGlobalMapping(Module &M, SmallVectorImpl<GlobalValue*> &Globals) {
      LLVMContext &C = M.getContext();
      SmallVector<Constant*, 8> Entries;

      Type* PtrTy = Type::getInt8PtrTy(C);
      StructType *EntryTy = StructType::get(C, {PtrTy, PtrTy}, true);

      for(GlobalValue* GV : Globals) {
        GlobalVariable* Name = getStringGlobal(M, GV->getName());
        Constant* NameCast = ConstantExpr::getPointerCast(Name, PtrTy);
        Constant* GVCast = GV;
        if(GV->getType() != PtrTy)
          GVCast = ConstantExpr::getPointerCast(GV, PtrTy);
        Constant* Entry = ConstantStruct::get(EntryTy, {NameCast, GVCast});
        Entries.push_back(Entry);
      }
      Entries.push_back(Constant::getNullValue(EntryTy));

      Constant* Init = ConstantArray::get(ArrayType::get(EntryTy, Entries.size()), Entries);
      return new GlobalVariable(M, Init->getType(), true,
                                GlobalVariable::PrivateLinkage,
                                Init, "global_mapping");
    }

    static SmallVector<GlobalVariable*, 8>
    embedBitcode(Module &M, SmallVectorImpl<Function*> &Funs) {
      SmallVector<GlobalVariable*, 8> Bitcode(Funs.size());
      for(size_t i = 0, n = Funs.size(); i != n; ++i)
        Bitcode[i] = embedBitcode(M, *Funs[i]);
      return Bitcode;
    }

    static GlobalVariable* embedBitcode(Module &M, Function& F) {
      std::unique_ptr<Module> Embed = CloneModule(&M);
      Function &FEmbed = *M.getFunction(F.getName());

      cleanModule(FEmbed, *Embed);

      dropLocalInitializers(*Embed);
      makeEverythingLocal(FEmbed, *Embed);

      return writeModuleToGlobal(M, FEmbed.getName() + "_bitcode");
    }

    static std::string moduleToString(Module &M) {
      std::string s;
      raw_string_ostream so(s);
      WriteBitcodeToFile(&M, so);
      so.flush();
      return s;
    }

    static GlobalVariable* writeModuleToGlobal(Module &M, Twine Name) {
      std::string Bitcode = moduleToString(M);
      Constant* BitcodeInit = ConstantDataArray::getString(M.getContext(), Bitcode, false);
      return new GlobalVariable(M, BitcodeInit->getType(), true,
                                GlobalVariable::PrivateLinkage,
                                BitcodeInit, Name);
    }

    static void cleanModule(Function &Entry, Module &M) {
      //clean the clonned module
      legacy::PassManager Passes;
      std::vector<GlobalValue*> Keep = {&Entry};
      Passes.add(createGVExtractionPass(Keep));
      Passes.add(createGlobalDCEPass());
      Passes.add(createStripDeadDebugInfoPass());
      Passes.add(createStripDeadPrototypesPass());
      Passes.run(M);
    }

    static void dropLocalInitializers(Module &M) {
      for(GlobalVariable& GV : M.globals())
        if(GV.hasLocalLinkage())
          GV.setInitializer(nullptr);
    }

    static void makeEverythingLocal(Function &Entry, Module &M) {

      Entry.setLinkage(Function::PrivateLinkage);
      llvm::appendToCompilerUsed(M, {&Entry});

      for(GlobalValue &GV : M.global_values()) {
        if(GV.hasLocalLinkage())
          continue;
        if(GV.getName().startswith("llvm."))
          continue;

        if(GV.isDefinitionExact() && !GV.isDeclaration()) {
          // privatize a copy
          GV.setLinkage(GlobalValue::PrivateLinkage);
        } else {
          // transform to declaration
          GV.setLinkage(GlobalValue::ExternalLinkage);

          if(Function* F = dyn_cast<Function>(&GV))
            F->getBasicBlockList().clear();
          if(GlobalVariable* GVar = dyn_cast<GlobalVariable>(&GV))
            GVar->setInitializer(nullptr);
        }
      }
    }

    Function* declareRegisterBitcode(Module &M, GlobalVariable *GlobalMapping) {
      StringRef Name = "easy_register";
      if(Function* F = M.getFunction(Name))
        return F;

      LLVMContext &C = M.getContext();
      DataLayout const &DL = M.getDataLayout();

      Type* Void = Type::getVoidTy(C);
      Type* I8Ptr = Type::getInt8PtrTy(C);
      Type* GMTy = GlobalMapping->getType();
      Type* SizeT = DL.getLargestLegalIntType(C);

      assert(SizeT);

      FunctionType* FTy =
          FunctionType::get(Void, {I8Ptr, I8Ptr, GMTy, I8Ptr, SizeT}, false);
      return Function::Create(FTy, Function::ExternalLinkage, Name, &M);
    }

    static void
    registerBitcode(Module &M, SmallVectorImpl<Function*> &Funs,
                    SmallVectorImpl<GlobalVariable*> &Bitcodes,
                    Value* GlobalMapping,
                    Function* RegisterBitcodeFun) {
      // Create static initializer with low priority to register everything
      Type* FPtr = RegisterBitcodeFun->getFunctionType()->getParamType(0);
      Type* StrPtr = RegisterBitcodeFun->getFunctionType()->getParamType(1);
      Type* BitcodePtr = RegisterBitcodeFun->getFunctionType()->getParamType(3);
      Type* SizeT = RegisterBitcodeFun->getFunctionType()->getParamType(4);

      Function *Ctor = getCtor(M);
      IRBuilder<> B(Ctor->getEntryBlock().getTerminator());

      for(size_t i = 0, n = Funs.size(); i != n; ++i) {
        GlobalVariable* Name = getStringGlobal(M, Funs[i]->getName());
        ArrayType* ArrTy = cast<ArrayType>(Bitcodes[i]->getInitializer()->getType());

        Value* Fun = B.CreatePointerCast(Funs[i], FPtr);
        Value* NameCast = B.CreatePointerCast(Name, StrPtr);
        Value* Bitcode = B.CreatePointerCast(Bitcodes[i], BitcodePtr);
        Value* BitcodeSize = ConstantInt::get(SizeT, ArrTy->getNumElements(), false);

        // fun, name, gm, bitcode, bitcode size
        B.CreateCall(RegisterBitcodeFun,
                     {Fun, NameCast, GlobalMapping, Bitcode, BitcodeSize}, "");
      }

      llvm::appendToGlobalCtors(M, Ctor, 65535);
    }

    static GlobalVariable* getStringGlobal(Module& M, StringRef Name) {
      Constant* Init = ConstantDataArray::getString(M.getContext(), Name, true);
      return new GlobalVariable(M, Init->getType(), true,
                                GlobalVariable::PrivateLinkage,
                                Init, Name + "_name");
    }

    static Function* getCtor(Module &M) {
      LLVMContext &C = M.getContext();
      Type* Void = Type::getVoidTy(C);
      FunctionType* VoidFun = FunctionType::get(Void, false);
      Function* Ctor = Function::Create(VoidFun, Function::PrivateLinkage, "register_bitcode", &M);
      BasicBlock* Entry = BasicBlock::Create(C, "entry", Ctor);
      ReturnInst::Create(C, Entry);
      return Ctor;
    }
  };

  char RegisterBitcode::ID = 0;
  static RegisterPass<RegisterBitcode> Register("easy-register-bitcode",
    "Parse the compilation unit and insert runtime library calls to register "
    "the bitcode associated to functions marked as \"jit\".",
                                                false, false);
}
