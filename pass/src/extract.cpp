#include "extract.hpp"

#include "llvm/Pass.h"
#include "llvm/InitializePasses.h"

#include "llvm/Support/raw_ostream.h"

#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/Bitcode/ReaderWriter.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"

//TODO: fix this correct include, should link to lib, not incldue cpp files
#include "lib/Transforms/IPO/ExtractGV.cpp"
#include "lib/Transforms/Utils/CloneModule.cpp"

#include <set>
#include <vector>
#include <string>

using namespace pass;
using namespace llvm;

namespace pass {

    bool debug = true;

    char ExtractFunctionToIR::ID = 0;

    ExtractFunctionToIR::ExtractFunctionToIR() : llvm::ModulePass(ID) {
    }

    ExtractFunctionToIR::~ExtractFunctionToIR() {
    }

    bool ExtractFunctionToIR::doInitialization(llvm::Module&) {
        return false;
    }

    bool ExtractFunctionToIR::doFinalization(llvm::Module&) {
        return false;
    }

    void ExtractFunctionToIR::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    }

    std::set<Function*> getFunctionsToExtract(Function* easy_jit_enabled) {
        assert(easy_jit_enabled);
        std::set<Function*> functions2extract;
        std::vector<CallInst*> calls;

        //collect the functions where the function is used
        for(User* use : easy_jit_enabled->users()) {
            CallInst* use_as_call = dyn_cast<CallInst>(use);
            assert(use_as_call);
            Function* parentFun = use_as_call->getParent()->getParent();
            assert(parentFun);
            functions2extract.insert(parentFun);
            calls.push_back(use_as_call);
        }

        //remove all the uses
        for(CallInst* call : calls)
            call->eraseFromParent();

        if(debug) {
            for(Function* f : functions2extract)
                errs() << "Function " << f->getName() << " marked for extraction.\n";
        }

        return functions2extract;
    }

    Function* AddJITHookDefinition(Module* M) {
        Function* jit_hook = M->getFunction("easy_jit_hook");

        if(!jit_hook) {
            Type* i8ptr = Type::getInt8PtrTy(M->getContext());
            Type* i64 = Type::getInt64Ty(M->getContext());

            FunctionType* hook_ty = FunctionType::get(i8ptr, std::vector<Type*>({i8ptr, i8ptr, i64}), false);

            jit_hook = Function::Create(hook_ty, Function::ExternalLinkage, "easy_jit_hook", M);
        }

        return jit_hook;
    }

    bool addGlobalIfUsedByExtracted(GlobalValue& gv, const std::set<Function*> functions2extract, std::set<GlobalValue*>* globals) {
        for(User* user : gv.users()) {
            Instruction* use_inst = dyn_cast<Instruction>(user);

            if(use_inst) {
                bool used_in_extracted = functions2extract.count(use_inst->getParent()->getParent());

                if(used_in_extracted) {
                    globals->insert(&gv);
                    return true;
                }
            }
        }

        return false;
    }

    std::set<GlobalValue*> getReferencedGlobals(const std::set<Function*>& functions2extract, Module* M) {
        std::set<GlobalValue*> globals;

        for(GlobalVariable& gv : M->globals()) {
            if(addGlobalIfUsedByExtracted(gv, functions2extract, &globals)) {
                if(debug) errs() << "Global variable " << gv.getName() << " referenced by extracted function.\n";
            }
        }

        for(Function& f : *M) {
            if(!functions2extract.count(&f)) {
                if(addGlobalIfUsedByExtracted(f, functions2extract, &globals)) {
                    if(debug) errs() << "Function " << f.getName() << " referenced by extracted function\n";
                }
            }
        }

        return globals;
    }

    bool ValidForExtraction(const std::set<GlobalValue*>& referencedGlobals) {
        for(GlobalValue* gv : referencedGlobals) {
            bool is_private = (gv->getLinkage() == GlobalValue::PrivateLinkage || gv->getLinkage() == GlobalValue::InternalLinkage);

            if(is_private) {
                if(debug) errs() << "Cannot extract module to ir since global " << gv->getName() << " has internal linkage.\n";

                return false;
            }
        }

        return true;
    }

    void CreateJITHook(Function* f) {
        std::string name = f->getName();

        Function* hook = Function::Create(f->getFunctionType(), f->getLinkage(), "hook", f->getParent());
        f->replaceAllUsesWith(hook);
        f->eraseFromParent();
        hook->setName(name);
        
        Module* M = hook->getParent();
        LLVMContext& context = hook->getContext();
        Type* i8ptr = Type::getInt8PtrTy(context);
        
        //get the hook function to the runtime
        Function* easy_jit_hook = M->getFunction("easy_jit_hook");
        assert(easy_jit_hook);
        
        //create a single block
        BasicBlock* entry = BasicBlock::Create(context, "entry", hook);
        
        //create a string
        ArrayType* i8array = ArrayType::get(cast<IntegerType>(Type::getInt8Ty(context)), name.size() + 1);
        Constant* initializer = ConstantDataArray::getString(context, name, true);
        Constant* function_name_global =
            new GlobalVariable(*M, initializer->getType(), true, GlobalValue::PrivateLinkage, initializer, ".easy_jit_fun_name");
        function_name_global = ConstantExpr::getPointerCast(cast<Constant>(function_name_global), i8ptr);
        
        //get the ir variable
        Constant* ir_variable = M->getNamedGlobal("easy_jit_module");
        assert(ir_variable);
        ArrayType* arrty = cast<ArrayType>(ir_variable->getType()->getContainedType(0));
        Constant* size = ConstantInt::get(Type::getInt64Ty(context), arrty->getNumElements());
        ir_variable = ConstantExpr::getPointerCast(cast<Constant>(ir_variable), i8ptr);
        
        //create the call
        std::vector<Value*> hook_args;
        hook_args.push_back(function_name_global);
        hook_args.push_back(ir_variable);
        hook_args.push_back(size);
        
        Value* jitResult = CallInst::Create(easy_jit_hook, hook_args, "jitted_function", entry);
        Value* cast = CastInst::CreatePointerCast(jitResult, hook->getType(), "jitted_function_cast", entry);

        std::vector<Value*> fun_args;
        for(Argument& arg : hook->args())
            fun_args.push_back(&arg);

        Value* call = CallInst::Create(cast, fun_args, "", entry);

        if(call->getType()->isVoidTy())
            ReturnInst::Create(context, entry); else {

            call->setName("result");
            ReturnInst::Create(context, call, entry);
        }
    }

    Module* getModuleForJITCompilation(const std::set<Function*>& functions2extract, Module* M) {
        std::set<GlobalValue*> referencedGlobals = getReferencedGlobals(functions2extract, M);

        if(!ValidForExtraction(referencedGlobals)) return nullptr;

        Module* Clone = llvm::CloneModule(M);
        
        //collect the referenced globals in the clone
        std::vector<GlobalValue*> globalsToKeep;

        for(GlobalValue* gv : referencedGlobals) {
            GlobalValue* gv_in_clone = Clone->getNamedGlobal(gv->getName());
            assert(gv_in_clone);
            globalsToKeep.push_back(gv_in_clone);
        }

        size_t number_of_globals_to_keep = globalsToKeep.size();

        //add the functions to the globalsToKeep
        for(Function* fun : functions2extract) {
            Function* fun_in_clone = Clone->getFunction(fun->getName());
            assert(fun_in_clone);
            globalsToKeep.push_back(fun_in_clone);
        }

        //clean the clonned module
        llvm::legacy::PassManager Passes;
        Passes.add(createGVExtractionPass(globalsToKeep));
        Passes.add(createGlobalDCEPass());
        Passes.add(createStripDeadDebugInfoPass());
        Passes.add(createStripDeadPrototypesPass());
        Passes.run(*Clone);

        //mark globals as external
        for(size_t i = 0; i < number_of_globals_to_keep; ++i) {
            GlobalValue* gv_in_clone = globalsToKeep[i];
            Function* gv_as_fun = dyn_cast<Function>(gv_in_clone);
            GlobalVariable* gv_as_global = dyn_cast<GlobalVariable>(gv_in_clone);

            if(gv_as_global) {
                gv_as_global->setLinkage(GlobalValue::ExternalLinkage);
                gv_as_global->setInitializer(nullptr);
            } else if(gv_as_fun) {
                gv_as_fun->setLinkage(GlobalValue::ExternalLinkage);
                gv_as_fun->getBasicBlockList().clear();
            } else assert(false);
        }

        return Clone;
    }

    std::string ModuleToString(Module* M) {
        std::string s;
        raw_string_ostream so(s);
        WriteBitcodeToFile(M, so);
        so.flush();
        return s;
    }

    GlobalVariable* WriteModuleToGlobal(Module* M, Module* jitM) {
        std::string module_as_string = ModuleToString(jitM);
        LLVMContext& context = M->getContext();
        
        ArrayType* type = ArrayType::get(cast<IntegerType>(Type::getInt8Ty(context)), module_as_string.size() + 1);
        Constant* initializer = ConstantDataArray::getString(context, module_as_string, true);
        GlobalVariable* bitcode_string =
            new GlobalVariable(*M, initializer->getType(), true, GlobalValue::PrivateLinkage, initializer, "easy_jit_module");

        if(debug)
            errs() << "Extracted module written to " << bitcode_string->getName() << "\n";

        return bitcode_string;
    }

    bool ExtractFunctionToIR::runOnModule(llvm::Module& M) {
        Function* easy_jit_enabled = M.getFunction("easy_jit_enabled");

        if(M.getNamedGlobal("easy_jit_module")) {
            errs() << "WARNING: Module already contains an extracted module.\n";
            return false;
        }

        if(easy_jit_enabled) {
            std::set<Function*> functions2extract = getFunctionsToExtract(easy_jit_enabled);

            if(functions2extract.empty()) return false;

            Module* jitM = getModuleForJITCompilation(functions2extract, &M);

            if(!jitM) return false;

            WriteModuleToGlobal(&M, jitM);
            AddJITHookDefinition(&M);

            //drop extracted functions
            for(Function* f : functions2extract)
                CreateJITHook(f);

            return true;
        }

        return false;
    }

    const char* command_line = "extract2ir";
    const char* description = "Pass to extract functions to ir.";
    bool is_analysis = false;
    bool is_cfg_only = false;

    static RegisterPass<ExtractFunctionToIR> X(command_line, description, is_cfg_only, is_analysis);
}
