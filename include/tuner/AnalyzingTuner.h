#ifndef TUNER_ANALYZING_TUNER
#define TUNER_ANALYZING_TUNER

#include <tuner/Tuner.h>

namespace tuner {

  class AnalyzingTuner : public Tuner {
  private:
    bool alreadyRun = false;

  public:

    AnalyzingTuner(KnobSet KS) : Tuner(KS) {}

    // collects knobs relevant for tuning from the module.
    void analyze(llvm::Module &M) override;

};

} // namespace tuner

#endif // TUNER_ANALYZING_TUNER
