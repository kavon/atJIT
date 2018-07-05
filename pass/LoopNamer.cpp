
#include <llvm/Analysis/LoopPass.h>

using namespace llvm;

namespace tuner {

class LoopNamer : public LoopPass {
public:
  static char ID;

  LoopNamer()
    : LoopPass(ID) {};

  bool runOnLoop(Loop *Loop, LPPassManager &LPM) override {
    // skip this loop if it already has an ID
    if (Loop->getLoopID() != nullptr)
      return false;

    LLVMContext &Context = Loop->getHeader()->getContext();

    // Reserve first location for self reference to the LoopID metadata node.
    MDNode *Dummy = MDNode::get(Context, {});
    MDNode *NewLoopID = MDNode::get(Context, {Dummy});

    // Set operand 0 to refer to the loop id itself.
    NewLoopID->replaceOperandWith(0, NewLoopID);

    Loop->setLoopID(NewLoopID);

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
