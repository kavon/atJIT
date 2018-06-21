#ifndef TUNER_TUNABLE_INLINER
#define TUNER_TUNABLE_INLINER

#include <tuner/Knob.h>

#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Analysis/InlineCost.h>
#include <llvm/Analysis/TargetTransformInfo.h>

namespace tuner {

  // NOTE: a better version of this knob would specialize on
  // a vector that represents all of the InlineParams fields.
  class TunableInliner : public Knob<int>, public llvm::LegacyInlinerBase {
    llvm::InlineParams Params;

    llvm::TargetTransformInfoWrapperPass *TTIWP;

  public:
    TunableInliner()
      : llvm::LegacyInlinerBase(ID),
        Params(llvm::getInlineParams()) {}

    TunableInliner(unsigned OptLevel, unsigned SizeOptLevel)
      : llvm::LegacyInlinerBase(ID),
        Params(llvm::getInlineParams(OptLevel, SizeOptLevel)) {}

    void setVal(int Threshold) override {
      Params.DefaultThreshold = Threshold;
    }

    int getVal() const override {
      return Params.DefaultThreshold;
    }

    /////////
    // the below need to be reimplemented because SimpleInliner
    // is not accessable for us to inherit.

    llvm::InlineCost getInlineCost(llvm::CallSite CS) override;
    bool runOnSCC(llvm::CallGraphSCC &SCC) override;
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

    static char ID;

  }; // end class

} // end namespace




#endif // TUNER_TUNABLE_INLINER
