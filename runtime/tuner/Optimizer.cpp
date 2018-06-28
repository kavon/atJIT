#include <tuple>
#include <iostream>

#include <tuner/optimizer.h>

#include <easy/runtime/Context.h>
#include <easy/runtime/BitcodeTracker.h>
#include <easy/runtime/RuntimePasses.h>

#include <tuner/TunableInliner.h>

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

  void Optimizer::setupPassManager() {
    // std::cout << "setup pass manager\n";
    auto &BT = easy::BitcodeTracker::GetTracker();
    const char* Name = BT.getName(Addr_);

    unsigned OptLevel;
    unsigned OptSize;
    std::tie(OptLevel, OptSize) = Cxt_->getOptLevel();

    llvm::Triple Triple{llvm::sys::getProcessTriple()};

    // get all knobs
    auto Inliner = tuner::createTunableInlinerPass(OptLevel, OptSize);

    // TODO remove this. setup tuner
    // auto Knobs = std::make_unique<IntKnobsTy>(10);
    // Knobs->push_back(static_cast<tuner::Knob<int>*>(Inliner));
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

    InitializedPassMgr_ = true;
  }

  Optimizer::Optimizer(void* Addr, std::shared_ptr<easy::Context> Cxt) :
      Cxt_(Cxt), Addr_(Addr), InitializedPassMgr_(false) {}

  easy::Context const* Optimizer::getContext() const {
    return Cxt_.get();
  }

  void* Optimizer::getAddr() const {
    return Addr_;
  }

  void Optimizer::optimize(llvm::Module &M) {
    // NOTE(kavon): right now we throw away the indicator saying whether
    // the module changed. Perhaps its useful to tell the tuner about that?
    // std::cout << "OPTIMIZING\n";
    if (!InitializedPassMgr_)
      setupPassManager();

    MPM_->run(M);
  }

} // namespace tuner