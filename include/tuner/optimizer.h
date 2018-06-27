#ifndef TUNER_OPTIMIZER
#define TUNER_OPTIMIZER

#include <easy/runtime/Context.h>

namespace llvm {
  namespace legacy {
    class PassManager;
  }
  class TargetMachine;
}

namespace tuner {

/////
// each Optimizer instance is an encapsulation of the state of
// dynamically optimizing a specific function.
//
// This consists of things like the knobs, tuner,
// feedback information, and context of the function.
class Optimizer {
private:
  easy::Context const& Cxt_;
  void* Addr_; // the function pointer

  // members related to the pass manager that we need to keep alive.
  std::unique_ptr<llvm::legacy::PassManager> MPM_;
  std::unique_ptr<llvm::TargetMachine> TM_;

  void setupPassManager();

public:
  Optimizer(void* Addr, easy::Context const& Cxt);

  easy::Context const& getContext() const;

  void* getAddr() const;

  void optimize(llvm::Module &M);

}; // end class

} // end namespace

#endif // TUNER_OPTIMIZER
