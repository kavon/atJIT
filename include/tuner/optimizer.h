#ifndef TUNER_OPTIMIZER
#define TUNER_OPTIMIZER

#include <list>

#include <dispatch/dispatch.h>

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetMachine.h>

#include <tuner/Tuner.h>
#include <tuner/Knob.h>
#include <tuner/KnobSet.h>
#include <tuner/Feedback.h>

namespace tuner {

  using CompileResult = std::pair<std::unique_ptr<easy::Function>, std::shared_ptr<tuner::Feedback>>;

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

  //////////
  // members related to concurrent JIT compilation

  // a serial compilation-job queue, and results list.
  dispatch_queue_t recompileQ_;
  std::list<CompileResult> recompileReady_;
  std::optional<CompileResult> obtainResult_;
  const size_t RECOMPILE_MAX = 10;

  /////////////

  void setupPassManager(KnobSet &);
  void findContextKnobs(KnobSet &);

  // members related to automatic tuning
  Tuner *Tuner_;

public:
  Optimizer(void* Addr, std::shared_ptr<easy::Context> Cxt, bool LazyInit = false);
  ~Optimizer();

  // the "lazy" initializer that must be called manually if LazyInit == true
  void initialize();

  easy::Context const* getContext() const;

  void* getAddr() const;

  void recompile_callback(bool canChain);
  void obtain_callback();

  CompileResult recompile();

}; // end class

} // end namespace

#endif // TUNER_OPTIMIZER
