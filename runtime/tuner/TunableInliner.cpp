
#include <tuner/TunableInliner.h>

///////
// other less-interesting methods / utilities of the TunableInliner.
//
// NOTE: much of this code was lifted from SimpleInliner in LLVM.

#define DEBUG_TYPE "tunable-inliner"

using namespace llvm;

#include <iostream>

InlineCost tuner::TunableInliner::getInlineCost(CallSite CS) {
    std::cout << "    calculating inline cost\n";
    Function *Callee = CS.getCalledFunction();
    TargetTransformInfo &TTI = TTIWP->getTTI(*Callee);

    bool RemarksEnabled = false;
    const auto &BBs = CS.getCaller()->getBasicBlockList();
    if (!BBs.empty()) {
      auto DI = OptimizationRemark(DEBUG_TYPE, "", DebugLoc(), &BBs.front());
      if (DI.isEnabled())
        RemarksEnabled = true;
    }
    OptimizationRemarkEmitter ORE(CS.getCaller());

    std::function<AssumptionCache &(Function &)> GetAssumptionCache =
        [&](Function &F) -> AssumptionCache & {
      return ACT->getAssumptionCache(F);
    };
    return llvm::getInlineCost(CS, Params, TTI, GetAssumptionCache,
                               /*GetBFI=*/None, PSI,
                               RemarksEnabled ? &ORE : nullptr);
  }

bool tuner::TunableInliner::runOnSCC(CallGraphSCC &SCC) {
  TTIWP = &getAnalysis<TargetTransformInfoWrapperPass>();
  return LegacyInlinerBase::runOnSCC(SCC);
}

void tuner::TunableInliner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetTransformInfoWrapperPass>();
  LegacyInlinerBase::getAnalysisUsage(AU);
}


///////////////
// Pass Registration and Initialization function.

char tuner::TunableInliner::ID = 0;
static RegisterPass<tuner::TunableInliner> X("tuned-inliner", "Tunable Function Integration/Inlining");

tuner::TunableInliner* tuner::createTunableInlinerPass(unsigned OptLevel, unsigned OptSize) {
  static tuner::TunableInliner* ThePass = nullptr;

  if (ThePass)
    return ThePass;

  // NOTE: do not change variable name "Registry" ,
  // the macros following that statement need it.
  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
  INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
  INITIALIZE_PASS_DEPENDENCY(ProfileSummaryInfoWrapperPass)
  INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
  INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
  return new tuner::TunableInliner(OptLevel, OptSize);
}
