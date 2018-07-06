#ifndef TUNER_KNOBSET
#define TUNER_KNOBSET

#include <tuner/Knob.h>
#include <tuner/LoopKnob.h>

#include <unordered_map>

namespace tuner {

  // using std::variant & std::visit to try and combine the
  // different instances of Knob into a single container
  // looks like a royal pain-in-the-butt:
  // https://en.cppreference.com/w/cpp/utility/variant/visit


  // NOTE: don't forget to update the following things
  // if any new knob kinds are added:
  //
  // 1. class KnobConfig
  // 2. Tuner::applyConfig
  // 3. Tuner::dumpConfig

  class KnobSet {
  public:
    std::unordered_map<KnobID, knob_type::Int*> IntKnobs;
    std::unordered_map<KnobID, knob_type::Loop*> LoopKnobs;

  };

}

#endif // TUNER_KNOBSET
