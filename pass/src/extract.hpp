#include "llvm/Pass.h"

#include "llvm/IR/Module.h"

class FunctionPass;

namespace pass {

    struct ExtractAndEmbed : public llvm::ModulePass {

        static char ID;

        ExtractAndEmbed();

        bool runOnModule(llvm::Module&) override;
    };

}
