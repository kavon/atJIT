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
    KnobSet KS_;
    std::mt19937 Gen_; // // 32-bit mersenne twister random number generator

  public:
    RandomTuner(KnobSet KS) : KS_(KS) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        Gen_ = std::mt19937(seed);
      }

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~RandomTuner() {}

    void applyConfiguration(std::shared_ptr<Feedback> IGNORED) override {
      for (auto Knob : KS_.IntKnobs) {
        std::uniform_int_distribution<> dist(Knob->min(), Knob->max());
        Knob->setVal(dist(Gen_));
      }
    }

  }; // end class RandomTuner

} // namespace tuner

#endif // TUNER_RANDOM_TUNER
