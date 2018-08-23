#include <tuple>
#include <iostream>

#include <tuner/optimizer.h>

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>
#include <easy/runtime/RuntimePasses.h>

#include <tuner/RandomTuner.h>
#include <tuner/BayesianTuner.h>
#include <tuner/AnnealingTuner.h>

#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO.h>

#ifdef POLLY_KNOBS
  #include <polly/RegisterPasses.h>
  #include <polly/Canonicalization.h>
#endif

namespace {
  static std::unique_ptr<llvm::TargetMachine> GetHostTargetMachine() {
    std::unique_ptr<llvm::TargetMachine> TM(llvm::EngineBuilder().selectTarget());
    return TM;
  }
} // end anonymous namespace

namespace tuner {

  bool Optimizer::haveInitPollyPasses_ = false;

  // generates a fresh PassManager to optimize the module.
  // This MPM should be generated _after_ the Tuner has applied a configuration
  // to the KnobSet!
  std::unique_ptr<llvm::legacy::PassManager> Optimizer::genPassManager() {
    auto &BT = easy::BitcodeTracker::GetTracker();
    const char* Name = BT.getName(Addr_);

    unsigned OptLevel = OptLvl.getVal();
    unsigned OptSize = OptSz.getVal();

    llvm::Triple Triple{llvm::sys::getProcessTriple()};

    llvm::PassManagerBuilder Builder;
    Builder.OptLevel = OptLevel;
    Builder.SizeLevel = OptSize;
    Builder.LibraryInfo = new llvm::TargetLibraryInfoImpl(Triple);
    Builder.Inliner = llvm::createFunctionInliningPass(InlineThresh.getVal());

#ifndef NDEBUG
    Builder.VerifyInput = true;
    Builder.VerifyOutput = true;
#endif

    TM_ = GetHostTargetMachine();
    assert(TM_);
    TM_->adjustPassManager(Builder);

    auto MPM = std::make_unique<llvm::legacy::PassManager>();

    MPM->add(llvm::createTargetTransformInfoWrapperPass(TM_->getTargetIRAnalysis()));

    // We absolutely must run this first!
    MPM->add(easy::createContextAnalysisPass(Cxt_));
    MPM->add(easy::createInlineParametersPass(Name));

    // After some cleanup etc, run devirtualization.
    Builder.addExtension(llvm::PassManagerBuilder::EP_ScalarOptimizerLate,
        [=] (auto const& Builder, auto &PM) {
          PM.add(easy::createDevirtualizeConstantPass(Name));
        });

#ifdef POLLY_KNOBS
    // Before the main optimizations, we want to run Polly
    Builder.addExtension(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
        [=] (auto const& Builder, auto &PM) {
          polly::registerCanonicalicationPasses(PM);
          polly::registerPollyPasses(PM);
        });
#endif

    // fill in the rest of the pass manager.
    Builder.populateModulePassManager(*MPM);

    return MPM;
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
    // first we need to make sure no concurrent compiles are still running
    if (recompileActive_) {
#ifndef NDEBUG
      std::cerr << "\nNOTE: optimizer's destructor is waiting for compile threads to finish.\n";
#endif

      while (recompileActive_)
        sleep_for(1);

#ifndef NDEBUG
      std::cerr << "NOTE: compile threads flushed.\n";
#endif
    }

    delete Tuner_;

    if (InitializedSelf_) {
      // we're not using ARC, so we need to manually deallocate the queues.
      dispatch_release(optimizeQ_);
      dispatch_release(codegenQ_);
      dispatch_release(mutate_recompileDone_);
    }
  }

  // the "lazy" initializer
  void Optimizer::initialize() {
    if (InitializedSelf_)
      return;

#ifndef NDEBUG
    std::cout << "initializing optimizer\n";
#endif

#ifdef POLLY_KNOBS
  if (!haveInitPollyPasses_) {
    haveInitPollyPasses_ = true;
    llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
    polly::initializePollyPasses(Registry);
  }
#endif

    /////////
    // initialize concurrency stuff
    optimizeQ_ = dispatch_queue_create("atJIT.optimizeQ", NULL);

    codegenQ_ = dispatch_queue_create("atJIT.codegenQ", NULL);

    // FIXME: we should be able to parallelize this queue.
    // one of the difficulties is that recompileActive can get
    // set to false before all recompile jobs are flushed.
    //
    // codegenQ_ = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0);

    mutate_recompileDone_ = dispatch_queue_create("atJIT.mutate_recompileDone", NULL);

    /////////
    // initialize the metadata needed for JIT compilation
    auto &BT = easy::BitcodeTracker::GetTracker();
    GMap_ = BT.getNameAndGlobalMapping(Addr_);

    /////////
    // collect knobs

    KnobSet KS;

    findContextKnobs(KS);

    //////////
    // IR Opt knobs

    // start at defaults, as requested by user in the Context.
    unsigned OptLevel;
    unsigned OptSize;
    std::tie(OptLevel, OptSize) = Cxt_->getOptLevel();

    InlineThresh = InlineThreshold(OptLevel, OptSize);
    OptLvl = OptimizerOptLvl(OptLevel);
    OptSz = OptimizerSizeLvl(OptSize);


    KS.IntKnobs[OptLvl.getID()] = &OptLvl;
    KS.IntKnobs[OptSz.getID()] = &OptSz;
    KS.IntKnobs[InlineThresh.getID()] = &InlineThresh;

    ////////////////////////////

    // Codegen knobs
    if (!isNoopTuner_) {
      // if we're not servicing a plain non-tuning JIT request,
      // make codegen as fast as possible by default.
      CGOptLvl = CodeGenOptLvl(1);
      FastISelOpt = FastISelOption(true);
    }

    KS.IntKnobs[CGOptLvl.getID()] = &CGOptLvl;
    KS.IntKnobs[FastISelOpt.getID()] = &FastISelOpt;


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
        isNoopTuner_ = true;
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

    void optimizeTask(void* P) {
      Optimizer* Opt = static_cast<Optimizer*>(P);
      Opt->optimize_callback();
    }

    void codegenTask(void* P) {
      OptimizeResult* OR = static_cast<OptimizeResult*>(P);
      OR->Opt->codegen_callback(OR);
      delete OR;
    }

    // list tasks

    void obtainResultTask(void* P) {
      RecompileRequest* R = static_cast<RecompileRequest*>(P);
      R->Opt->obtain_callback(R);
    }

    void addResultTask(void* P) {
      AddCompileResult* ACR = static_cast<AddCompileResult*>(P);
      ACR->Opt->addToList_callback(ACR);
    }

  } // end extern C

/////////////////////////////////

  // adds a result to the list from the waiting queue.
  // NOTE: must be holding mutate_recompileDone_ queue!
  void Optimizer::addToList_callback(AddCompileResult* ACR) {
    recompileDone_.push_back(std::move(ACR->Result));
    doneQueueEmpty_ = false;
  }

  // tries once to pop a compile result off the ready list.
  // result is placed into obtainResult_, which is
  // only to be accessed by the main thread.
  void Optimizer::obtain_callback(RecompileRequest* R) {
    assert(!R->RetVal.has_value() && "should be None");

    if (recompileDone_.empty()) {
      doneQueueEmpty_ = true;
      R->RetVal = std::nullopt;
      return;
    }

    assert(recompileDone_.size() > 0);

    R->RetVal = std::move(recompileDone_.front());
    recompileDone_.pop_front();

    doneQueueEmpty_ = recompileDone_.empty();
  }

  opt_status::Value Optimizer::status() const {
    if (doneQueueEmpty_) {
      if (recompileActive_)
        return opt_status::Working;
      else
        return opt_status::Empty;
    }
    return opt_status::Ready;
  }


  //////////////////////
  /// STARTING POINT
  //////////////////////
  CompileResult Optimizer::recompile() {
#ifndef NDEBUG
    auto Start = std::chrono::system_clock::now();
#endif
    RecompileRequest R;
    R.Opt = this;
    R.RetVal = std::nullopt;

    // check the list for already-compiled versions, blocking.
    dispatch_sync_f(mutate_recompileDone_, &R, obtainResultTask);

    if (R.RetVal.has_value() == false) {

      if (recompileActive_ == false) {
        // start a compile job
        recompileActive_ = true;
        dispatch_async_f(optimizeQ_, this, optimizeTask);
      }

      // wait for the first job to finish.
      const unsigned msDuration = 1;
      uint64_t waitIters = 0;
      do {
        if (msDuration * waitIters >= COMPILE_JOB_BAILOUT_MS) {
          std::cerr << "\n\nERROR: A compile job took too long, aborting!!\n"
                    << std::flush;

          // we can't cancel any threads or perform cleanup,
          // so we must forcefully terminate the program.
          // See here for discussion: https://github.com/kavon/atJIT/issues/4
          std::abort();
        }

        // wait a bit
        sleep_for(msDuration);
        waitIters++;

        // check for a result
        dispatch_sync_f(mutate_recompileDone_, &R, obtainResultTask);
      } while (!R.RetVal.has_value());
    }

#ifndef NDEBUG
    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);
    std::cout << ">> reoptimize request finished in " << elapsed.count() << " ms\n";
#endif

    assert(R.RetVal.has_value() && "that's wrong");
    return std::move(R.RetVal.value());
  }


  // must be dispatched from optimizeQ_.
  // this function will start a chain of recompile jobs
  // up to the predefined max.
  // it will then queue a new job that pushes the result
  // onto the list. Thus, holding the list queue will deadlock
  // this compile queue!
  void Optimizer::optimize_callback() {
#ifndef NDEBUG
    auto Start = std::chrono::system_clock::now();
#endif

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

#ifndef NDEBUG
    std::cout << "@@ optimize job starting using this config:\n";
    dumpConfig(Tuner_->getKnobSet(), *TunerConf);

    Tuner_->dump();
#endif

    // save the current compilation config to pass it
    // along to the codegen thread.
    auto CGLevel = CGOptLvl.getLevel();
    auto FastISel = FastISelOpt.getFlag();

    //////////////////////

    // Optimize the IR.
    auto MPM = genPassManager();
    MPM->run(*M);

    easy::Function::WriteOptimizedToFile(*M, Cxt_->getDebugFile());

#ifndef NDEBUG
    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);
    std::cout << "@@ optimize job finished in " << elapsed.count() << " ms\n";
#endif

    // ask the tuner if we're able to, and *should* try
    // compiling the next config ahead-of-time.
    bool shouldCompile = Tuner_->shouldCompileNext();

    if (shouldCompile) {
      // start an async recompile job
      dispatch_async_f(optimizeQ_, this, optimizeTask);
    }

    OptimizeResult* OR = new OptimizeResult();
    OR->M = std::move(M);
    OR->LLVMCxt = std::move(LLVMCxt);
    OR->FB = std::move(FB);
    OR->Opt = this;
    OR->End = !shouldCompile;
    OR->CGLevel = CGLevel;
    OR->FastISel = FastISel;

    // start an async codegen job
    dispatch_async_f(codegenQ_, OR, codegenTask);
  }



  void Optimizer::codegen_callback(OptimizeResult* OR) {
#ifndef NDEBUG
    auto Start = std::chrono::system_clock::now();
#endif

    const char* Name;
    easy::GlobalMapping* Globals;
    std::tie(Name, Globals) = GMap_;

    // Compile to assembly.
    std::unique_ptr<easy::Function> Fun =
        easy::Function::CompileAndWrap(
            Name, Globals, std::move(OR->LLVMCxt), std::move(OR->M), OR->CGLevel,
            OR->FastISel);

    AddCompileResult ACR;
    ACR.Opt = this;
    ACR.Result = {std::move(Fun), std::move(OR->FB)};

    dispatch_sync_f(mutate_recompileDone_, &ACR, addResultTask);

#ifndef NDEBUG
    auto End = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start);
    std::cout << "$$ codegen job finished in " << elapsed.count() << " ms\n";
#endif

    // am I the end of the chain?
    if (OR->End)
      recompileActive_ = false;
  }

} // namespace tuner
