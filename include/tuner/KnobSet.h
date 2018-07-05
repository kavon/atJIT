#ifndef TUNER_KNOBSET
#define TUNER_KNOBSET

#include <tuner/Knob.h>

#include <unordered_map>

namespace tuner {

  // using std::variant & std::visit to try and combine the
  // different instances of Knob into a single container
  // looks like a royal pain-in-the-butt:
  // https://en.cppreference.com/w/cpp/utility/variant/visit


  // NOTE: don't forget to update Tuner::applyConfig if
  // any new knob kinds are added here!
  class KnobSet {
  public:
    std::unordered_map<tuner::KnobID, knob_type::Int*> IntKnobs;

  };

}

#endif // TUNER_KNOBSET
