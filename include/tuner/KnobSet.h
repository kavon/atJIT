#ifndef TUNER_KNOBSET
#define TUNER_KNOBSET

#include <tuner/Knob.h>

namespace tuner {

  // using std::variant & std::visit to try and combine the
  // different instances of Knob into a single container
  // looks like a royal pain-in-the-butt:
  // https://en.cppreference.com/w/cpp/utility/variant/visit

  class KnobSet {
  public:
    std::vector<knob_type::Int*> IntKnobs;

  };

}

#endif // TUNER_KNOBSET
