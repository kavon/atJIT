
#include <tuner/LoopKnob.h>
#include <tuner/MDUtils.h>

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/IR/LegacyPassManager.h>

namespace tuner {

  namespace {

    using namespace llvm;

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
        if (LoopName != LK->getLoopID())
          return false;

        // TODO: change the metadata.
        errs() << "adjustor for loop " << LK->getLoopID() << " would have fired\n";

        return false;
      }

    }; // end class

    char LoopKnobAdjustor::ID = 0;
    static RegisterPass<LoopKnobAdjustor> Register("loop-knob-adjustor",
      "Adjust the metadata of a single loop",
                            false /* only looks at CFG*/,
                            false /* analysis pass */);

  } // end anonymous namespace

void LoopKnob::apply (llvm::Module &M) {
  legacy::PassManager Passes;
  Passes.add(new LoopKnobAdjustor(this));
  Passes.run(M);
}

} // end namespace tuner
