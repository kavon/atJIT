#ifndef TUNER_ANNEALING_TUNER
#define TUNER_ANNEALING_TUNER

#include <tuner/RandomTuner.h>

namespace tuner {

  // a tuner that uses simulated annealing
  class AnnealingTuner : public RandomTuner {
    float temperature = 100.0;
  public:
    AnnealingTuner(KnobSet KS, std::shared_ptr<easy::Context> Cxt)
      : RandomTuner(KS, std::move(Cxt)) {}

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~AnnealingTuner() {}

    GenResult& getNextConfig() override {
      KnobConfig KC;

      // if this is the first requested config, we generate the default config.
      if (Configs_.empty())
        KC = genDefaultConfig(KS_);


      // TODO: implement an acceptance probability function
      // P(e, e', T), where
      //     e = quality of the best config
      //     e' = quality of the candidate config
      //     T = temperature of the system.
      //
      // according to wikipedia, one such P is:
      //
      //  if e' < e
      //    then 1
      //    else exp(-(e'-e)/T)
      //
      // more research is needed into various algorithms for this,
      // as there are things like Adaptive Simulated Annealing, etc.

      auto Conf = std::make_shared<KnobConfig>(KC);
      auto FB = std::make_shared<ExecutionTime>(Cxt_->getFeedbackStdErr());

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      return Configs_.back();
    }

  }; // end class RandomTuner

} // namespace tuner

#endif // TUNER_ANNEALING_TUNER
