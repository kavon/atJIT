
#include <tuner/AnalyzingTuner.h>
#include <tuner/LoopKnob.h>

namespace tuner {

void AnalyzingTuner::analyze(llvm::Module &M){
  // only run this once, since the input module
  // is fixed for each instance of a Tuner.
  if (alreadyRun)
    return;
  alreadyRun = true;

  std::cout << "LOOK FOR LOOPS AND MAKE KNOBS HERE\n";

}

} // end namespace
