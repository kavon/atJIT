#ifndef MAY_ALIAS_TRACER
#define MAY_ALIAS_TRACER

#include <llvm/ADT/SmallPtrSet.h>

namespace llvm {
  class Value;
  class GlobalObject;
}

namespace easy {
  class MayAliasTracer {
    llvm::SmallPtrSet<llvm::GlobalObject*, 32> GOs_;

    using VSet = llvm::SmallPtrSetImpl<llvm::Value*>;
    void mayAliasWithStoredValues(llvm::Value* V, VSet &Loaded, VSet &Stored);
    void mayAliasWithLoadedValues(llvm::Value* V, VSet &Loaded, VSet &Stored);


    public:

    MayAliasTracer(llvm::Value* V) {
      llvm::SmallPtrSet<llvm::Value*, 32> VLoaded;
      llvm::SmallPtrSet<llvm::Value*, 32> VStored;
      mayAliasWithLoadedValues(V, VLoaded, VStored);
    }
    auto count(llvm::GlobalObject& GO) const { return GOs_.count(&GO);}
  };

}

#endif
