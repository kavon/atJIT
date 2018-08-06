#ifndef TUNER_OPTIMIZER
#define TUNER_OPTIMIZER

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>

#include <tuner/Tuner.h>
#include <tuner/Knob.h>
#include <tuner/KnobSet.h>
#include <tuner/Feedback.h>

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

  // metadata about the function being compiled
  std::tuple<const char*, easy::GlobalMapping*> GMap_;

  // members related to the pass manager that we need to keep alive.
  std::unique_ptr<llvm::legacy::PassManager> MPM_;
  std::unique_ptr<llvm::TargetMachine> TM_;
  bool InitializedSelf_;

  void setupPassManager(KnobSet &);
  void findContextKnobs(KnobSet &);

  // members related to automatic tuning
  Tuner *Tuner_;

  std::shared_ptr<Feedback> optimize(llvm::Module &M);

public:
  Optimizer(void* Addr, std::shared_ptr<easy::Context> Cxt, bool LazyInit = false);
  ~Optimizer();

  // the "lazy" initializer that must be called manually if LazyInit == true
  void initialize();

  easy::Context const* getContext() const;

  void* getAddr() const;

  std::pair<std::unique_ptr<easy::Function>,
            std::shared_ptr<tuner::Feedback>> recompile();

}; // end class

} // end namespace

#endif // TUNER_OPTIMIZER
