#include <tuple>
#include <chrono>
#include <thread>
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

  void sleep_for(unsigned ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
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
    // first we need to make sure no concurrent compiles still running
    while (recompileActive_)
      sleep_for(1);

    delete Tuner_;
  }

  // the "lazy" initializer
  void Optimizer::initialize() {
    if (InitializedSelf_)
      return;

    std::cout << "initializing optimizer\n";

    /////////
    // initialize concurrency stuff
    optimizeQ_ = dispatch_queue_create("atJIT.optimizeQ", NULL);
    codegenQ_ = dispatch_queue_create("atJIT.codegenQ", NULL);
    mutate_recompileDone_ = dispatch_queue_create("atJIT.mutate_recompileDone", NULL);

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
      Opt->recompile_callback();
    }

    void codegenTask(void* P) {
      OptimizeResult* OR = static_cast<OptimizeResult*>(P);
      OR->Opt->codegen_callback(OR);
      delete OR;
    }

    // list tasks

    void obtainResultTask(void* P) {
      Optimizer* Opt = static_cast<Optimizer*>(P);
      Opt->obtain_callback();
    }

    void addResultTask(void* P) {
      Optimizer* Opt = static_cast<Optimizer*>(P);
      Opt->addToList_callback();
    }

  } // end extern C

/////////////////////////////////

  // adds a result to the list from the waiting queue.
  // NOTE: must be holding BOTH codegenQ_ and mutate_recompileDone_ queue!
  void Optimizer::addToList_callback() {
    assert(toBeAdded_.has_value());

    auto R = std::move(toBeAdded_.value());
    toBeAdded_ = std::nullopt;

    recompileDone_.push_back(std::move(R));
  }

  // tries once to pop a compile result off the ready list.
  // result is placed into obtainResult_, which is
  // only to be accessed by the main thread.
  void Optimizer::obtain_callback() {
    assert(!obtainResult_.has_value() && "should be None");

    if (recompileDone_.empty()) {
      obtainResult_ = std::nullopt;
      return;
    }

    assert(recompileDone_.size() > 0);

    obtainResult_ = std::move(recompileDone_.front());
    recompileDone_.pop_front();
  }


  //////////////////////
  /// STARTING POINT
  //////////////////////
  CompileResult Optimizer::recompile() {
    auto Start = std::chrono::system_clock::now();

    // check the list for already-compiled versions, blocking.
    dispatch_sync_f(mutate_recompileDone_, this, obtainResultTask);

    if (obtainResult_.has_value() == false) {

      if (recompileActive_ == false) {
        // start a compile job
        recompileActive_ = true;
        dispatch_async_f(optimizeQ_, this, recompileTask);
      }

      // wait for the first job to finish.
      do {
        sleep_for(5);
        dispatch_sync_f(mutate_recompileDone_, this, obtainResultTask);
      } while (!obtainResult_.has_value());
    }

    assert(obtainResult_.has_value() && "that's wrong");

    auto R = std::move(obtainResult_.value());
    obtainResult_ = std::nullopt;

    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);
    std::cout << ">> reoptimize request finished in " << elapsed.count() << " ms\n";

    return R;
  }


  // must be dispatched from optimizeQ_.
  // this function will start a chain of recompile jobs
  // up to the predefined max.
  // it will then queue a new job that pushes the result
  // onto the list. Thus, holding the list queue will deadlock
  // this compile queue!
  void Optimizer::recompile_callback() {

    auto Start = std::chrono::system_clock::now();

    auto &BT = easy::BitcodeTracker::GetTracker();

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

    // Tuner_->dump();

    //////////////////////

    // Optimize the IR.
    // This also may not be thread-safe unless if we reconstruct the MPM.
    MPM_->run(*M);

    easy::Function::WriteOptimizedToFile(*M, Cxt_->getDebugFile());

    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);
    std::cout << "@@ optimize job finished in " << elapsed.count() << " ms\n";

    // ask the tuner if we're able to, and *should* try
    // compiling the next config ahead-of-time.
    bool shouldCompile = Tuner_->shouldCompileNext();

    if (shouldCompile) {
      // start an async recompile job
      dispatch_async_f(optimizeQ_, this, recompileTask);
    }

    OptimizeResult* OR = new OptimizeResult(this, std::move(FB), std::move(M), std::move(LLVMCxt), !shouldCompile);

    // start an async codegen job
    dispatch_async_f(codegenQ_, OR, codegenTask);
  }



  void Optimizer::codegen_callback(OptimizeResult* OR) {
    auto Start = std::chrono::system_clock::now();

    const char* Name;
    easy::GlobalMapping* Globals;
    std::tie(Name, Globals) = GMap_;

    // Compile to assembly.
    std::unique_ptr<easy::Function> Fun =
        easy::Function::CompileAndWrap(
            Name, Globals, std::move(OR->LLVMCxt), std::move(OR->M));

    // it's a pain to pass two values to the callback.
    assert(toBeAdded_.has_value() == false);
    toBeAdded_ = {std::move(Fun), std::move(OR->FB)};

    dispatch_sync_f(mutate_recompileDone_, this, addResultTask);

    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);
    std::cout << "$$ codegen job finished in " << elapsed.count() << " ms\n";

    // am I the end of the chain?
    if (OR->End)
      recompileActive_ = false;
  }

} // namespace tuner
