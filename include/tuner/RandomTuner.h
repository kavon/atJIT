#ifndef TUNER_RANDOM_TUNER
#define TUNER_RANDOM_TUNER

#include <tuner/AnalyzingTuner.h>
#include <tuner/LoopKnob.h>

#include <iostream>
#include <random>
#include <chrono>

namespace tuner {

  // a tuner that randomly perturbs its knobs
  class RandomTuner : public AnalyzingTuner {
    std::mt19937 Gen_; // // 32-bit mersenne twister random number generator

  public:
    RandomTuner(KnobSet KS) : AnalyzingTuner(KS) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        Gen_ = std::mt19937(seed);
      }

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~RandomTuner() {}

    GenResult& getNextConfig() override {
      KnobConfig KC;

      // if this is the first requested config, we generate the default config.
      if (Configs_.empty())
        KC = genDefaultConfig(KS_);
      else
        KC = genRandomConfig(KS_, Gen_);

      auto Conf = std::make_shared<KnobConfig>(KC);
      auto FB = std::make_shared<ExecutionTime>();

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      return Configs_.back();
    }

  }; // end class RandomTuner

} // namespace tuner

#endif // TUNER_RANDOM_TUNER
