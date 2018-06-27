#ifndef TUNER_OPTIMIZER
#define TUNER_OPTIMIZER

#include <tuple>

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>

#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>

namespace {
  static std::unique_ptr<llvm::TargetMachine> GetHostTargetMachine() {
    std::unique_ptr<llvm::TargetMachine> TM(llvm::EngineBuilder().selectTarget());
    return TM;
  }
} // end anonymous namespace

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

    void setupPassManager() {
      auto &BT = easy::BitcodeTracker::GetTracker();
      const char* Name = BT.getName(Addr_);

      unsigned OptLevel;
      unsigned OptSize;
      std::tie(OptLevel, OptSize) = Cxt_.getOptLevel();

      llvm::Triple Triple{llvm::sys::getProcessTriple()};

      // get all knobs
      auto Inliner = tuner::createTunableInlinerPass(OptLevel, OptSize);

      // setup tuner
      auto Knobs = std::make_unique<IntKnobsTy>(10);
      Knobs->push_back(static_cast<tuner::Knob<int>*>(Inliner));
      // C.initializeTuner(std::move(Knobs)); // TODO

      llvm::PassManagerBuilder Builder_;
      Builder_.OptLevel = OptLevel;
      Builder_.SizeLevel = OptSize;
      Builder_.LibraryInfo = new llvm::TargetLibraryInfoImpl(Triple);
      Builder_.Inliner = Inliner;

      TM_ = GetHostTargetMachine();
      assert(TM_);
      TM_->adjustPassManager(Builder_);

      MPM_ = std::make_unique<llvm::legacy::PassManager>();
      MPM_->add(llvm::createTargetTransformInfoWrapperPass(TM_->getTargetIRAnalysis()));
      MPM_->add(easy::createContextAnalysisPass(Cxt_));
      MPM_->add(easy::createInlineParametersPass(Name));
      Builder_.populateModulePassManager(*MPM_);
      MPM_->add(easy::createDevirtualizeConstantPass(Name));
      Builder_.populateModulePassManager(*MPM_);
    }

  public:
    Optimizer(void* Addr, easy::Context const& Cxt) : Cxt_(Cxt), Addr_(Addr) {
      setupPassManager();
    }

    easy::Context const& getContext() const {
      return Cxt_;
    }

    void* getAddr() const {
      return Addr_;
    }

    void optimize(llvm::Module &M) {
      // NOTE(kavon): right now we throw away the indicator saying whether
      // the module changed. Perhaps its useful to tell the tuner about that?
      MPM_->run(M);
    }

  }; // end class Optimizer

} // namespace tuner

#endif // TUNER_OPTIMIZER
