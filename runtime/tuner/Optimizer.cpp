#include <tuple>
#include <iostream>

#include <tuner/optimizer.h>

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>
#include <easy/runtime/RuntimePasses.h>

#include <tuner/TunableInliner.h>
#include <tuner/RandomTuner.h>
#include <tuner/BayesianTuner.h>
#include <tuner/AnnealingTuner.h>

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

    // FIXME: would rather not run the whole pass sequence twice!
    MPM_->add(easy::createDevirtualizeConstantPass(Name));
    Builder_.populateModulePassManager(*MPM_);

  }

  void Optimizer::findContextKnobs(KnobSet &KS) {
    for (std::shared_ptr<easy::ArgumentBase> AB : *Cxt_) {
      switch(AB->kind()) {

        case easy::ArgumentBase::AK_IntRange: {
          auto const* IntArg = AB->as<easy::IntRangeArgument>();
          auto *ScalarKnob = static_cast<knob_type::ScalarInt*>(IntArg->get());
          KS.IntKnobs[ScalarKnob->getID()] = ScalarKnob;
        } break;

        case easy::ArgumentBase::AK_Forward:
        case easy::ArgumentBase::AK_Int:
        case easy::ArgumentBase::AK_Float:
        case easy::ArgumentBase::AK_Ptr:
        case easy::ArgumentBase::AK_Struct:
        case easy::ArgumentBase::AK_Module: {
          continue;
        } break;
      };

    }
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

    std::cout << "initializing optimizer\n";

    /////////
    // initialize concurrency stuff
    recompileQ_ = dispatch_queue_create("atJIT.recompileQueue", NULL);

    /////////
    // initialize the metadata needed for JIT compilation
    auto &BT = easy::BitcodeTracker::GetTracker();
    GMap_ = BT.getNameAndGlobalMapping(Addr_);

    /////////
    // collect knobs

    KnobSet KS;

    setupPassManager(KS);

    findContextKnobs(KS);


    /////////
    // create tuner

    switch (Cxt_->getTunerKind()) {

      case tuner::AT_Bayes:
        Tuner_ = new BayesianTuner(std::move(KS), Cxt_);
        break;

      case tuner::AT_Random:
        Tuner_ = new RandomTuner(std::move(KS), Cxt_);
        break;

      case tuner::AT_Anneal:
        Tuner_ = new AnnealingTuner(std::move(KS), Cxt_);
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


//////////////////////////////////

  // for proper standards compliance, we need to mark our
  // GCD call-back functions with C linkage.
  extern "C" {
    // NOTE: this task is only safe in a serial job queue.
    void recompileTask(void* P) {
      Optimizer* Opt = static_cast<Optimizer*>(P);
      Opt->recompile_callback(true);
    }

    void obtainResultTask(void* P) {
      Optimizer* Opt = static_cast<Optimizer*>(P);
      Opt->obtain_callback();
    }

  } // end extern C

/////////////////////////////////

  void Optimizer::obtain_callback() {
    assert(!obtainResult_.has_value() && "should be None");

    if (recompileReady_.empty()) {
      // we need the result asap, so we do it here
      // since we're holding the semaphore for the recompile
      // callback.
      recompile_callback(false);
    }

    assert(recompileReady_.size() > 0);

    obtainResult_ = std::move(recompileReady_.front());
    recompileReady_.pop_front();

    // FIXME launch a chained recompile job!
  }

  CompileResult Optimizer::recompile() {

    // the user program needs a new version right now.
    dispatch_sync_f(recompileQ_, this, obtainResultTask);

    assert(obtainResult_.has_value() && "that's wrong");

    auto R = std::move(obtainResult_.value());
    obtainResult_ = std::nullopt;

    return R;
  }


  void Optimizer::recompile_callback(bool canChain) {
    std::cout << "recompiling...\n";
    auto Start = std::chrono::system_clock::now();

    auto &BT = easy::BitcodeTracker::GetTracker();

    const char* Name;
    easy::GlobalMapping* Globals;
    std::tie(Name, Globals) = GMap_;

    std::unique_ptr<llvm::Module> M;
    std::unique_ptr<llvm::LLVMContext> LLVMCxt;
    std::tie(M, LLVMCxt) = BT.getModule(Addr_);

    easy::Function::WriteOptimizedToFile(*M, Cxt_->getDebugBeforeFile());

    /////////////////////
    // allow the tuner to analyze & modify the IR.
    Tuner_->analyze(*M);
    auto Gen = Tuner_->getNextConfig();
    auto TunerConf = Gen.first;
    auto FB = Gen.second;

    Tuner_->applyConfig(*TunerConf, *M);

    Tuner_->dump();
    //////////////////////

    // Optimize the IR.
    // This also may not be thread-safe unless if we reconstruct the MPM.
    MPM_->run(*M);

    easy::Function::WriteOptimizedToFile(*M, Cxt_->getDebugFile());

    // Compile to assembly.
    std::unique_ptr<easy::Function> Fun =
        easy::Function::CompileAndWrap(
            Name, Globals, std::move(LLVMCxt), std::move(M));

    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);
    std::cout << "done in " << elapsed.count() << " ms\n";

    recompileReady_.push_back({std::move(Fun), std::move(FB)});

  }

} // namespace tuner
