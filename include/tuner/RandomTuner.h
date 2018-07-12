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

      {
        /////////////////////////
        // UNROLLING KNOBS

        // roll a 100-sided die, so we can make a weighted choice.
        std::uniform_int_distribution<unsigned> diceRoll(0, 100);
        unsigned unrollChoice = diceRoll(Eng);
        if (unrollChoice <= 10) {
          LS.UnrollDisable = true;
        } else if (unrollChoice <= 50) {
          LS.UnrollFull = true;
        } else {
          // pick a power-of-two (>= 2) unrolling factor
          std::uniform_int_distribution<unsigned> bitNum(1, 12); // 2^12 = 4096
          LS.UnrollCount = ((uint16_t) 1) << bitNum(Eng);
        }
      }

      {
        /////////////////////////
        // VECTORIZATION KNOBS

        std::uniform_int_distribution<unsigned> diceRoll1(0, 100);
        unsigned vecChoice = diceRoll1(Eng);

        if (vecChoice <= 30) {
          LS.VectorizeEnable = false;
        } else if (vecChoice <= 75) {
          LS.VectorizeEnable = true;
        } // otherwise, we don't specify anything.

        if (LS.VectorizeEnable && LS.VectorizeEnable.value()) {
          std::uniform_int_distribution<unsigned> diceRoll2(0, 100);
          // should we specify a width? otherwise, LLVM chooses automatically.
          if (diceRoll2(Eng) >= 40) {
            // pick a power-of-two (>= 2) width
            std::uniform_int_distribution<unsigned> bitNum(1, 5); // 2^5 = 32
            LS.VectorizeWidth = ((uint16_t) 1) << bitNum(Eng);
          }
        }
      }

      return LS;
    }
  } // end namespace

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
