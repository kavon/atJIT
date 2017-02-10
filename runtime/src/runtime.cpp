#include "llvm/InitializePasses.h"

#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/TargetSelect.h"

#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <iostream>
#include <map>

using namespace llvm;

namespace runtime {

class InitNativeTarget {
    public:
    InitNativeTarget() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
    }
};

static InitNativeTarget init;

class ModuleCache {
    std::map<const char*, std::shared_ptr<llvm::Module>> modules;

    llvm::Module* loadModule(const char* m, size_t size) {

        llvm::StringRef bytecode_as_string(m, size - 1);
        llvm::LLVMContext& context = llvm::getGlobalContext();
        std::unique_ptr<llvm::MemoryBuffer> memory_buffer(llvm::MemoryBuffer::getMemBuffer(bytecode_as_string));
        llvm::ErrorOr<std::unique_ptr<llvm::Module>> ModuleOrErr = llvm::parseBitcodeFile(memory_buffer.get()->getMemBufferRef(), context);

        if(std::error_code EC = ModuleOrErr.getError())
        {
            std::cerr << EC.message() << "\n";
            assert(false && "loading the module failed.");
        }

        std::unique_ptr<llvm::Module> ptr = std::move(ModuleOrErr.get());
        return ptr.release();
    }

    public:
    std::unique_ptr<llvm::Module> getModule(const char* m, size_t size) {
        bool in_cache = modules.count(m);

        if(!in_cache) modules[m] = std::shared_ptr<Module>(loadModule(m, size));

        return std::unique_ptr<Module>(CloneModule(modules[m].get()));
    }
};

std::unique_ptr<llvm::ExecutionEngine> getExecutionEngine(std::unique_ptr<Module> M) {
    EngineBuilder ebuilder(std::move(M));
    std::string eeError;

    auto ee = std::move(std::unique_ptr<llvm::ExecutionEngine>(ebuilder.setErrorStr(&eeError)
            .setMCPU(sys::getHostCPUName())
            .setEngineKind(EngineKind::JIT)
            .setOptLevel(llvm::CodeGenOpt::Level::Aggressive)
            .create()));

    return ee;
}

static ModuleCache moduleCache;
static std::unique_ptr<llvm::ExecutionEngine> exec_engine;
}

using namespace runtime;


extern "C" void* easy_jit_hook(const char* function_name, const char* module_in_ir, size_t module_in_ir_size) {

    std::unique_ptr<llvm::Module> M = std::move(moduleCache.getModule(module_in_ir, module_in_ir_size));
    exec_engine = std::move(getExecutionEngine(std::move(M)));

    std::string jit_function_name = std::string(function_name) + "__";
    void* function_address = (void*)exec_engine->getFunctionAddress(jit_function_name.c_str());

    return function_address;
}

extern "C" void easy_jit_hook_end(void* function) {
    exec_engine = nullptr;
}
