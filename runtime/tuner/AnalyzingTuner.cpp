
#include <tuner/AnalyzingTuner.h>
#include <tuner/LoopKnob.h>

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/IR/LegacyPassManager.h>

namespace tuner {

namespace {

  using namespace llvm;

  class LoopKnobCreator : public LoopPass {
  private:
    KnobSet *KS;
  public:
    static char ID;

    // required to have a default ctor
    LoopKnobCreator() : LoopPass(ID), KS(nullptr) {}

    LoopKnobCreator(KnobSet *KSet)
      : LoopPass(ID), KS(KSet) {};

    bool runOnLoop(Loop *Loop, LPPassManager &LPM) override {
      MDNode *Name = Loop->getLoopID();

      if (Name == nullptr) {
        // LoopNamer should have been run when embedding the bitcode.
        llvm_unreachable("encountered an unnamed loop!");
        return false;
      }

      LoopKnob *LK = new LoopKnob(Name->getMetadataID());

      KS->LoopKnobs[LK->getID()] = LK;

      return false;
    }

  }; // end class

  char LoopKnobCreator::ID = 0;
  static RegisterPass<LoopKnobCreator> Register("loop-knob-creator",
    "Collect the names of all loops to make knobs.",
                          false /* only looks at CFG*/,
                          false /* analysis pass */); // NOTE it kind of is analysis...

} // end anonymous namespace


void AnalyzingTuner::analyze(llvm::Module &M) {
  // only run this once, since the input module
  // is fixed for each instance of a Tuner.
  if (alreadyRun)
    return;
  alreadyRun = true;

  legacy::PassManager Passes;
  Passes.add(new LoopKnobCreator(&KS_));
  Passes.run(M);

}

} // end namespace


// this file seemed fitting to define this function. can't put in header.
std::ostream& operator<<(std::ostream &o, tuner::LoopSetting &LS) {
  o << "<Loop Setting Config Here>";
  return o;
}
