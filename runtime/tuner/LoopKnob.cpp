
#include <tuner/LoopKnob.h>
#include <tuner/MDUtils.h>

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/IR/LegacyPassManager.h>

namespace tuner {

  namespace {

    using namespace llvm;

    // {!"option.name"}, aka, no value such as an i1
    MDNode* setPresenceOption(MDNode *LMD, std::optional<bool> const& Option, StringRef OptName) {
      if (Option) {
        if (Option.value()) // add the option
          return updateLMD(LMD, OptName, nullptr);
        else // delete any such options, if present
          return updateLMD(LMD, OptName, nullptr, true);
      }

      return LMD;
    }

    MDNode* addToLoopMD(MDNode *LMD, LoopSetting const& LS) {
      LLVMContext &C = LMD->getContext();
      IntegerType* i32 = IntegerType::get(C, 32);

      LMD = setPresenceOption(LMD, LS.UnrollDisable, loop_md::UNROLL_DISABLE);
      LMD = setPresenceOption(LMD, LS.UnrollFull, loop_md::UNROLL_FULL);

      if (LS.UnrollCount)
        LMD = updateLMD(LMD, loop_md::UNROLL_COUNT,
                              mkMDInt(i32, LS.UnrollCount.value()));

      return LMD;
    }

    class LoopKnobAdjustor : public LoopPass {
    private:
      knob_type::Loop *LK;
    public:
      static char ID;

      // required to have a default ctor
      LoopKnobAdjustor() : LoopPass(ID), LK(nullptr) {}

      LoopKnobAdjustor(knob_type::Loop* LKnob)
        : LoopPass(ID), LK(LKnob) {};

      bool runOnLoop(Loop *Loop, LPPassManager &LPM) override {
        MDNode* LoopMD = Loop->getLoopID();
        if (!LoopMD) {
          // LoopNamer should have been run when embedding the bitcode.
          report_fatal_error("encountered a loop without llvm.loop metadata!");
          return false;
        }

        unsigned LoopName = getLoopName(LoopMD);

        // is this the right loop?
        if (LoopName != LK->getLoopName())
          return false;

        MDNode *NewLoopMD = addToLoopMD(LoopMD, LK->getVal());

        if (NewLoopMD == LoopMD) // then nothing changed.
          return false;

        Loop->setLoopID(NewLoopMD);

        return true;
      }

    }; // end class

    char LoopKnobAdjustor::ID = 0;
    static RegisterPass<LoopKnobAdjustor> Register("loop-knob-adjustor",
      "Adjust the metadata of a single loop",
                            false /* only looks at CFG*/,
                            false /* analysis pass */);

  } // end anonymous namespace

void LoopKnob::apply (llvm::Module &M) {
  // NOTE: ideally, we wouldn't run a pass over every loop in
  // the module for each call to this method.
  //
  // It would be better if (maybe)
  // each LoopKnob held onto the reference to the Loop*
  // we got from analysis, and simply mutated _that_ during
  // an invocation of this method. I believe this would work
  // if the Loop* is stable throughout the tuning process.

  legacy::PassManager Passes;
  Passes.add(new LoopKnobAdjustor(this));
  Passes.run(M);
}

} // end namespace tuner
