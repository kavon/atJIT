#include "llvm/Pass.h"

#include "llvm/IR/Module.h"

class FunctionPass;

namespace pass {

    struct ExtractFunctionToIR : public llvm::ModulePass {

        static char ID;

        ExtractFunctionToIR();
        virtual ~ExtractFunctionToIR();

        virtual bool doInitialization(llvm::Module&);
        virtual bool doFinalization(llvm::Module&);

        virtual void getAnalysisUsage(llvm::AnalysisUsage&) const;

        virtual bool runOnModule(llvm::Module&);
    };

}
