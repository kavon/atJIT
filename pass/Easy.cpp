#include <easy/attributes.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/IRBuilder.h>

#include <llvm/IR/LegacyPassManager.h>

#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <llvm/Transforms/Utils/CtorUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <llvm/Bitcode/BitcodeWriter.h>

#include <llvm/ADT/SetVector.h>

#define DEBUG_TYPE "easy-register-bitcode"
#include <llvm/Support/Debug.h>

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Regex.h>

#include <llvm/Support/raw_ostream.h>

#include <memory>

using namespace llvm;

static cl::opt<std::string> RegexString("easy-regex", cl::desc("<regex>"), cl::init(""));

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

    static auto compilerInterface(Module &M) {
      SmallVector<std::reference_wrapper<Function>, 4> Funs;
      std::copy_if(M.begin(), M.end(), std::back_inserter(Funs),
                   [](Function &F) {return F.getSection() == CI_SECTION;});
      return Funs;
    }

    static void collectFunctionsToJIT(Module &M, SmallVectorImpl<Function*> &FunsToJIT) {

      // get **all** functions passed as parameter to easy jit calls
      //   not only the target function, but also its parameters
      deduceFunctionsToJIT(M);
      regexFunctionsToJIT(M);

      // get functions in section jit section
      for(Function &F : M) {
        if(F.getSection() != JIT_SECTION)
          continue;

        F.setSection(""); // drop the identifier

        std::string Reason;
        if(!canExtractBitcode(F, Reason)) {
          DEBUG(dbgs() << "Could not extract function '" << F.getName() << "'. " << Reason << "\n");
          continue;
        }

        DEBUG(dbgs() << "Function '" << F.getName() << "' marked for extraction.\n");

        FunsToJIT.push_back(&F);
      }
    }

    static void deduceFunctionsToJIT(Module &M) {
      for(Function &EasyJitFun : compilerInterface(M)) {
        for(User* U : EasyJitFun.users()) {
          if(CallSite CS{U}) {
            for(Value* O : CS.args()) {
              O = O->stripPointerCastsNoFollowAliases();
              if(Function* F = dyn_cast<Function>(O)) {
                F->setSection(JIT_SECTION);
              }
            }
          }
        }
      }
    }

    static void regexFunctionsToJIT(Module &M) {
      if(RegexString.empty())
        return;
      llvm::Regex Match(RegexString);
      for(Function &F : M)
        if(Match.match(F.getName()))
          F.setSection(JIT_SECTION);
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

      Function &FEmbed = *Embed->getFunction(F.getName());
      cleanModule(FEmbed, *Embed);

      Twine ModuleName = F.getName() + "_bitcode";
      Embed->setModuleIdentifier(ModuleName.str());

      return writeModuleToGlobal(M, *Embed, FEmbed.getName() + "_bitcode");
    }

    static std::string moduleToString(Module &M) {
      std::string s;
      raw_string_ostream so(s);
      WriteBitcodeToFile(&M, so);
      so.flush();
      return s;
    }

    static GlobalVariable* writeModuleToGlobal(Module &M, Module &Embed, Twine Name) {
      std::string Bitcode = moduleToString(Embed);
      Constant* BitcodeInit = ConstantDataArray::getString(M.getContext(), Bitcode, true);
      return new GlobalVariable(M, BitcodeInit->getType(), true,
                                GlobalVariable::PrivateLinkage,
                                BitcodeInit, Name);
    }

    static void cleanModule(Function &Entry, Module &M) {
      Entry.setLinkage(GlobalValue::ExternalLinkage);
      auto Referenced = getReferencedFromEntry(Entry);

      //clean the clonned module
      legacy::PassManager Passes;
      Passes.add(createGVExtractionPass(Referenced));
      Passes.add(createGlobalDCEPass());
      Passes.add(createStripDeadDebugInfoPass());
      Passes.add(createStripDeadPrototypesPass());
      Passes.run(M);

      fixLinkages(Entry, M);
    }

    static std::vector<GlobalValue*> getReferencedFromEntry(Function &Entry) {
      std::vector<GlobalValue*> Funs;

      SmallPtrSet<User*, 32> Visited;
      SmallVector<User*, 8> ToVisit;
      ToVisit.push_back(&Entry);

      while(!ToVisit.empty()) {
        User* U = ToVisit.pop_back_val();
        if(!Visited.insert(U).second)
          continue;
        if(Function* UF = dyn_cast<Function>(U)) {
          Funs.push_back(UF);

          for(Instruction &I : instructions(UF))
            for(Value* Op : I.operands())
              if(User* OpU = dyn_cast<User>(Op))
                ToVisit.push_back(OpU);
        }

        for(Value* Op : U->operands())
          if(User* OpU = dyn_cast<User>(Op))
            ToVisit.push_back(OpU);
      }

      return Funs;
    }

    static void fixLinkages(Function &Entry, Module &M) {
      for(GlobalValue &GV : M.global_values()) {

        GV.setUnnamedAddr(GlobalValue::UnnamedAddr::None);
        if(Function* F = dyn_cast<Function>(&GV))
          F->removeFnAttr(Attribute::NoInline);

        if(&GV == &Entry)
          continue;
        if(GV.getName().startswith("llvm."))
          continue;
        if(GV.getVisibility() != GlobalValue::DefaultVisibility ||
           GV.getLinkage() != GlobalValue::PrivateLinkage) {
          GV.setVisibility(GlobalValue::DefaultVisibility);
          GV.setLinkage(GlobalValue::PrivateLinkage);
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
      Type* SizeTy = RegisterBitcodeFun->getFunctionType()->getParamType(4);

      Function *Ctor = getCtor(M);
      IRBuilder<> B(Ctor->getEntryBlock().getTerminator());

      for(size_t i = 0, n = Funs.size(); i != n; ++i) {
        GlobalVariable* Name = getStringGlobal(M, Funs[i]->getName());
        ArrayType* ArrTy = cast<ArrayType>(Bitcodes[i]->getInitializer()->getType());
        size_t Size = ArrTy->getNumElements()-1; /*-1 for the 0 terminator*/

        Value* Fun = B.CreatePointerCast(Funs[i], FPtr);
        Value* NameCast = B.CreatePointerCast(Name, StrPtr);
        Value* Bitcode = B.CreatePointerCast(Bitcodes[i], BitcodePtr);
        Value* BitcodeSize = ConstantInt::get(SizeTy, Size, false);

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

  llvm::Pass* createRegisterBitcodePass() {
    return new RegisterBitcode();
  }
}

