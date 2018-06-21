#ifndef TUNER_TUNABLE_INLINER
#define TUNER_TUNABLE_INLINER

#include <tuner/Knob.h>

#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Analysis/InlineCost.h>
#include <llvm/Analysis/TargetTransformInfo.h>

namespace tuner {

  using namespace llvm;

  // NOTE: a better version of this knob would specialize on
  // a vector that represents all of the InlineParams fields.
  class TunableInliner : public Knob<int>, public LegacyInlinerBase {
    char ID = 0;
    InlineParams Params;
    std::unique_ptr<Pass> Inliner;

    TargetTransformInfoWrapperPass *TTIWP;

  public:
    TunableInliner(unsigned OptLevel, unsigned SizeOptLevel)
      : LegacyInlinerBase(ID),
        Params(getInlineParams(OptLevel, SizeOptLevel)),
        Inliner(createFunctionInliningPass(Params)) {}

    void setVal(int Threshold) override {
      Params.DefaultThreshold = Threshold;
      Inliner.reset(createFunctionInliningPass(Params));
    }

    int getVal() const override {
      return Params.DefaultThreshold;
    }

    /////////
    // the below need to be reimplemented because SimpleInliner
    // is not accessable for us to inherit.

    InlineCost getInlineCost(CallSite CS) override;
    bool runOnSCC(CallGraphSCC &SCC) override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;

  }; // end class

} // end namespace

#endif // TUNER_TUNABLE_INLINER
