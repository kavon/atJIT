#ifndef TUNER_RANDOM_TUNER
#define TUNER_RANDOM_TUNER

#include <tuner/AnalyzingTuner.h>

#include <iostream>
#include <random>
#include <chrono>

namespace tuner {

  namespace {
    template < typename RNE >  // RandomNumberEngine
    LoopSetting genRandomLoopSetting(RNE &Eng) {
      LoopSetting LS;

      std::uniform_int_distribution<> unrollRange(0, 100);

      LS.UnrollCount = unrollRange(Eng);

      return LS;
    }
  }

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
      auto Conf = std::make_shared<KnobConfig>();
      auto FB = std::make_shared<ExecutionTime>();

      // if this is the first requested config, we do not
      // add anything to the knob config to test default settings.
      if (!Configs_.empty()) {
        // TODO: factor out common structure below with lambdas

        for (auto Entry : KS_.IntKnobs) {
          auto Knob = Entry.second;
          std::uniform_int_distribution<> dist(Knob->min(), Knob->max());
          Conf->IntConfig.push_back({Knob->getID(), dist(Gen_)});
        }

        for (auto Entry : KS_.LoopKnobs) {
          auto Knob = Entry.second;
          auto Setting = genRandomLoopSetting(Gen_);
          Conf->LoopConfig.push_back({Knob->getID(), Setting});
        }
      }

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      return Configs_.back();
    }

  }; // end class RandomTuner

} // namespace tuner

#endif // TUNER_RANDOM_TUNER
