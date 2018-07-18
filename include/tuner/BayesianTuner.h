#ifndef TUNER_BAYES_TUNER
#define TUNER_BAYES_TUNER

#include <tuner/AnalyzingTuner.h>

namespace tuner {

  // a tuner that uses Bayesian Optimization to find good configurations.
  class BayesianTuner : public AnalyzingTuner {

  public:
    BayesianTuner(KnobSet KS) : AnalyzingTuner(KS) { }

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~BayesianTuner() {}

    GenResult& getNextConfig() override {
      KnobConfig KC = genDefaultConfig(KS_);

      auto Conf = std::make_shared<KnobConfig>(KC);
      auto FB = std::make_shared<ExecutionTime>();

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      return Configs_.back();
    }

  }; // end class BayesianTuner

} // namespace tuner

#endif // TUNER_BAYES_TUNER
