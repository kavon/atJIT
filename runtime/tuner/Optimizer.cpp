#include <tuple>
#include <iostream>

#include <tuner/optimizer.h>

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>
#include <easy/runtime/RuntimePasses.h>

#include <tuner/TunableInliner.h>
#include <tuner/RandomTuner.h>
#include <tuner/BayesianTuner.h>

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

  void Optimizer::setupPassManager(KnobSet &KS) {
    auto &BT = easy::BitcodeTracker::GetTracker();
    const char* Name = BT.getName(Addr_);

    unsigned OptLevel;
    unsigned OptSize;
    std::tie(OptLevel, OptSize) = Cxt_->getOptLevel();

    llvm::Triple Triple{llvm::sys::getProcessTriple()};

    auto Inliner = tuner::createTunableInlinerPass(OptLevel, OptSize);
    KS.IntKnobs[Inliner->getID()] = Inliner;

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

    setupPassManager(KS);


    /////////
    // create tuner

    switch (Cxt_->getTunerKind()) {

      case tuner::AT_Bayes:
        Tuner_ = new BayesianTuner(std::move(KS));
        break;

      case tuner::AT_Random:
        Tuner_ = new RandomTuner(std::move(KS));
        break;

      case tuner::AT_None:
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

    std::cout << "\nREOPTIMIZING\n";

    auto Start = std::chrono::system_clock::now();

    std::cout << "analyzing module...\n";

    Tuner_->analyze(M);

    std::cout << "generating a config...\n";

    auto Gen = Tuner_->getNextConfig();
    auto TunerConf = Gen.first;
    auto FB = Gen.second;

    Tuner_->applyConfig(*TunerConf, M);

    std::cout << "compiling...\n";

    MPM_->run(M);

    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);

    Tuner_->dump();

    std::cout << "Finished in " << elapsed.count() << "ms\n"
              << "+++++++\n";

    return FB;
  }

} // namespace tuner
