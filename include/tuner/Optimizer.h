#ifndef TUNER_OPTIMIZER
#define TUNER_OPTIMIZER

#include <tuple>

#include <easy/runtime/Context.h>

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

  public:
    Optimizer(easy::Context const& Cxt) : Cxt_(Cxt) {

    }

    easy::Context const& getContext() const {
      return Cxt_;
    }

    void optimize(llvm::Module &M, const char* Name) {
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

      llvm::PassManagerBuilder Builder;
      Builder.OptLevel = OptLevel;
      Builder.SizeLevel = OptSize;
      Builder.LibraryInfo = new llvm::TargetLibraryInfoImpl(Triple);
      Builder.Inliner = Inliner;

      std::unique_ptr<llvm::TargetMachine> TM = GetHostTargetMachine();
      assert(TM);
      TM->adjustPassManager(Builder);

      llvm::legacy::PassManager MPM;
      MPM.add(llvm::createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
      MPM.add(easy::createContextAnalysisPass(Cxt_));
      MPM.add(easy::createInlineParametersPass(Name));
      Builder.populateModulePassManager(MPM);
      MPM.add(easy::createDevirtualizeConstantPass(Name));
      Builder.populateModulePassManager(MPM);

      MPM.run(M);
    }

  }; // end class Optimizer

} // namespace tuner

#endif // TUNER_OPTIMIZER
