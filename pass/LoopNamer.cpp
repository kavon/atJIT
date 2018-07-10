
#include <tuner/MDUtils.h>

#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/Constants.h>

using namespace llvm;

namespace tuner {

class LoopNamer : public LoopPass {
private:
  unsigned LoopIDs = 0;
public:
  static char ID;

  LoopNamer()
    : LoopPass(ID) {};

  bool runOnLoop(Loop *Loop, LPPassManager &LPM) override {
    MDNode *LoopMD = Loop->getLoopID();
    LLVMContext &Context = Loop->getHeader()->getContext();

    if (!LoopMD) {
      // Setup the first location with a dummy operand for now.
      MDNode *Dummy = MDNode::get(Context, {});
      LoopMD = MDNode::get(Context, {Dummy});
    }

    MDNode* KnobTag = createLoopName(Context, LoopIDs);
    MDNode *Wrapper = MDNode::get(Context, {KnobTag});

    // combine the knob tag with the current LoopMD.
    LoopMD = MDNode::concatenate(LoopMD, Wrapper);

    // reinstate the self-loop in the first position of the MD.
    LoopMD->replaceOperandWith(0, LoopMD);

    Loop->setLoopID(LoopMD);

    return true;
  }

}; // end class

char LoopNamer::ID = 0;
static RegisterPass<LoopNamer> Register("loop-namer",
  "Ensure every loop has a name (!llvm.loop metadata)",
                        false /* only looks at CFG*/,
                        false /* analysis pass */);

llvm::Pass* createLoopNamerPass() {
  return new LoopNamer();
}

} // end namespace
