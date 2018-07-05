#include <tuple>
#include <iostream>

#include <tuner/optimizer.h>

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>
#include <easy/runtime/RuntimePasses.h>

#include <tuner/TunableInliner.h>
#include <tuner/RandomTuner.h>

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

  knob_type::Int* Optimizer::setupPassManager() {
    auto &BT = easy::BitcodeTracker::GetTracker();
    const char* Name = BT.getName(Addr_);

    unsigned OptLevel;
    unsigned OptSize;
    std::tie(OptLevel, OptSize) = Cxt_->getOptLevel();

    llvm::Triple Triple{llvm::sys::getProcessTriple()};

    // get all knobs
    auto Inliner = tuner::createTunableInlinerPass(OptLevel, OptSize);

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

    return Inliner;
  }

  // currently, this constructor is called every time we query the ATDriver's
  // state map, because of how unordered_map is setup. Thus, you'll want to
  // call "initialize" once the object is known to be needed to actually
  // get it setup, since the work is non-trivial.
  Optimizer::Optimizer(void* Addr,
                       std::shared_ptr<easy::Context> Cxt,
                       bool LazyInit)
                       : Cxt_(Cxt), Addr_(Addr), InitializedSelf_(false),
                         Tuner_(nullptr) {
        if (!LazyInit)
          initialize();
      }

  Optimizer::~Optimizer() {
    delete Tuner_;
  }

  // the "lazy" initializer
  void Optimizer::initialize() {
    if (InitializedSelf_)
      return;

      // steps:
      // 1. collect all knobs
      // 2. initialize the desired tuner with those knobs.

    std::cout << "initializing optimizer\n";

    /////////
    // collect knobs

    KnobSet KS;

    auto K = setupPassManager();
    KS.IntKnobs[K->getID()] = K;

    /////////
    // create tuner

    switch (Cxt_->getTunerKind()) {

      case easy::AT_Random:
        Tuner_ = new RandomTuner(std::move(KS));
        break;

      case easy::AT_None:
      default:
        Tuner_ = new NoOpTuner(std::move(KS));
    }

    InitializedSelf_ = true;
  }

  easy::Context const* Optimizer::getContext() const {
    return Cxt_.get();
  }

  void* Optimizer::getAddr() const {
    return Addr_;
  }

  std::shared_ptr<Feedback> Optimizer::optimize(llvm::Module &M) {
    // NOTE(kavon): right now we throw away the indicator saying whether
    // the module changed. Perhaps its useful to store that in the Feedback?

    std::cout << "... optimizing\n";

    auto Gen = Tuner_->getNextConfig();
    auto TunerConf = Gen.first;
    auto FB = Gen.second;

    Tuner_->applyConfig(*TunerConf);

    MPM_->run(M);

    Tuner_->dump();

    return FB;
  }

} // namespace tuner
