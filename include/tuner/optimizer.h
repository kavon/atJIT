#ifndef TUNER_OPTIMIZER
#define TUNER_OPTIMIZER

#include <easy/runtime/Context.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>

#include <tuner/Tuner.h>

namespace tuner {

/////
// each Optimizer instance is an encapsulation of the state of
// dynamically optimizing a specific function.
//
// This consists of things like the knobs, tuner,
// feedback information, and context of the function.
class Optimizer {
private:
  std::shared_ptr<easy::Context> Cxt_;
  void* Addr_; // the function pointer

  // members related to the pass manager that we need to keep alive.
  std::unique_ptr<llvm::legacy::PassManager> MPM_;
  std::unique_ptr<llvm::TargetMachine> TM_;
  bool InitializedSelf_;

  void setupPassManager();

  // members related to automatic tuning
  Tuner *Tuner_;

public:
  Optimizer(void* Addr, std::shared_ptr<easy::Context> Cxt, bool LazyInit = false);
  ~Optimizer();

  // the "lazy" initializer that must be called manually if LazyInit == true
  void initialize();

  easy::Context const* getContext() const;

  void* getAddr() const;

  void optimize(llvm::Module &M);

}; // end class

} // end namespace

#endif // TUNER_OPTIMIZER
