#ifndef TUNER_RANDOM_TUNER
#define TUNER_RANDOM_TUNER

#include <tuner/Tuner.h>
#include <tuner/KnobSet.h>

#include <iostream>
#include <random>
#include <chrono>

namespace tuner {

  // a tuner that randomly perturbs its knobs
  class RandomTuner : public Tuner {
    std::mt19937 Gen_; // // 32-bit mersenne twister random number generator


  public:
    RandomTuner(KnobSet KS) : Tuner(KS) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        Gen_ = std::mt19937(seed);
      }

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~RandomTuner() {}

    GenResult& getNextConfig() override {
      auto Conf = std::make_shared<KnobConfig>();
      auto FB = std::make_shared<ExecutionTime>();

      for (auto Entry : KS_.IntKnobs) {
        auto Knob = Entry.second;
        std::uniform_int_distribution<> dist(Knob->min(), Knob->max());
        Conf->IntConfig.push_back({Knob->getID(), dist(Gen_)});
      }

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      return Configs_.back();
    }

  }; // end class RandomTuner

} // namespace tuner

#endif // TUNER_RANDOM_TUNER
