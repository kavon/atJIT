#ifndef TUNER_TUNABLE_INLINER
#define TUNER_TUNABLE_INLINER

#include <tuner/Knob.h>

#include <llvm/Transforms/IPO/Inliner.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Analysis/InlineCost.h>
#include <llvm/Analysis/TargetTransformInfo.h>

namespace tuner {

  // TODO: a better version of this knob would expose
  // a vector that represents all of the InlineParams fields.
  class TunableInliner : public ScalarKnob<int>, public llvm::LegacyInlinerBase {
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

    int min() const override {
      return -1000;
    }

    int max() const override {
      return 1000;
    }

    std::string getName() const override {
      return "inlining threshold";
    }

    // the LLVM module already has a reference to this object, and will "apply"
    // it implicitly when it calls getInlineCost, which accesses the Params
    // member.
    void apply(llvm::Module &M) override { }

    /////////
    // the below need to be reimplemented because SimpleInliner
    // is not accessable for us to inherit.

    llvm::InlineCost getInlineCost(llvm::CallSite CS) override;
    bool runOnSCC(llvm::CallGraphSCC &SCC) override;
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

    static char ID;

  }; // end class

  // use this instead of constructing the pass directly.
  // NOTE: its likely to break things if you call this more than once.
  TunableInliner* createTunableInlinerPass(unsigned OptLevel, unsigned OptSize);

} // end namespace




#endif // TUNER_TUNABLE_INLINER
