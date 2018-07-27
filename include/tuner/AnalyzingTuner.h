#ifndef TUNER_ANALYZING_TUNER
#define TUNER_ANALYZING_TUNER

#include <tuner/Tuner.h>
#include <easy/runtime/Context.h>

namespace tuner {

  class AnalyzingTuner : public Tuner {
  private:
    bool alreadyRun = false;

  protected:
    std::shared_ptr<easy::Context> Cxt_;

  public:

    AnalyzingTuner(KnobSet KS, std::shared_ptr<easy::Context> Cxt)
        : Tuner(KS), Cxt_(std::move(Cxt)) {}

    // collects knobs relevant for tuning from the module.
    void analyze(llvm::Module &M) override;

};

} // namespace tuner

#endif // TUNER_ANALYZING_TUNER
