
#include <tuner/TunableInliner.h>

///////
// other less-interesting methods of TunableInliner.
// NOTE: this code was lifted from SimpleInliner in LLVM.

#define DEBUG_TYPE "tunable-inliner"

using namespace llvm;

InlineCost tuner::TunableInliner::getInlineCost(CallSite CS) {
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
