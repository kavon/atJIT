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

  using CompileResult =
   std::pair<std::unique_ptr<easy::Function>, std::shared_ptr<tuner::Feedback>>;

  struct RecompileRequest {
    Optimizer* Opt;
    std::optional<CompileResult> RetVal;
  };

  struct AddCompileResult {
    Optimizer* Opt;
    CompileResult Result;
  };

  struct OptimizeResult {
  public:
    std::unique_ptr<llvm::Module> M;
    std::unique_ptr<llvm::LLVMContext> LLVMCxt;
    std::shared_ptr<tuner::Feedback> FB;
    Optimizer* Opt;
    bool End;

    OptimizeResult(Optimizer* Opt_, std::shared_ptr<tuner::Feedback> FB_, std::unique_ptr<llvm::Module> M_, std::unique_ptr<llvm::LLVMContext> LLVMCxt_,
    bool End_)
      :
      M(std::move(M_)), LLVMCxt(std::move(LLVMCxt_)), FB(std::move(FB_)), Opt(Opt_), End(End_) {}
  };

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

  // the initial job queue for recompile requests.
  // it is a serial queue that optimizes the IR.
  dispatch_queue_t optimizeQ_;

  // a serial job queue for IR -> asm compilation
  dispatch_queue_t codegenQ_;
  bool recompileActive_ = false;

  // serial list-access queues. The dispatch
  // queue is basically a semaphore.
  dispatch_queue_t mutate_recompileDone_;
  std::list<CompileResult> recompileDone_;



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

  //// these callbacks are a bit ugly.
  void addToList_callback(AddCompileResult*);
  void optimize_callback();
  void codegen_callback(OptimizeResult*);
  void obtain_callback(RecompileRequest*);

  CompileResult recompile();

}; // end class

} // end namespace

#endif // TUNER_OPTIMIZER
