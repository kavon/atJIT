#ifndef TUNER_RANDOM_TUNER
#define TUNER_RANDOM_TUNER

#include <tuner/Tuner.h>

#include <iostream>

using IntKnobsTy = std::vector<tuner::Knob<int>*>;
using UniqueIntKnobsTy = std::unique_ptr<IntKnobsTy>;

namespace tuner {

  // a silly tuner that randomly perturbs the knobs
  class RandomTuner : public Tuner {


    UniqueIntKnobsTy knobs_;

  public:

    RandomTuner(UniqueIntKnobsTy knobs) : knobs_(std::move(knobs)) {
      std::cout << "the RandomTuner was constructed\n";
    }
    ~RandomTuner() {}

    void applyConfiguration(Feedback &prior) override {
      // go over the knobs and set them randomly
      std::cout << "TODO: implement applyConfiguration\n";
      return;
    }

  }; // end class RandomTuner

} // namespace tuner

#endif // TUNER_RANDOM_TUNER
