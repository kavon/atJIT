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
  //
  // instead, we use abstract function-objects to implement the equivalent
  // of a lambda-case in a functional language, e.g., (\x -> case x of ...)
  // and write our own generic operations over them.


  // NOTE if you add another structure member, you must immediately update:
  //
  // 0. class KnobConfig
  // 1. KnobSetAppFn and related abstract visitors.
  // 2. applyToKnobs and related generic operations.

  class KnobSet {
  public:
    std::unordered_map<KnobID, knob_type::Int*> IntKnobs;
    std::unordered_map<KnobID, knob_type::Loop*> LoopKnobs;

  };

  // applies some arbitrary operation to a knob
  class KnobSetAppFn {
  public:
      virtual void operator()(std::pair<KnobID, knob_type::Int*>) = 0;
      virtual void operator()(std::pair<KnobID, knob_type::Loop*>) = 0;
  };

 void applyToKnobs(KnobSetAppFn &F, KnobSet const &KS);


}

#endif // TUNER_KNOBSET
