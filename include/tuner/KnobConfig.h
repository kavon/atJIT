#ifndef TUNER_KNOBCONFIG
#define TUNER_KNOBCONFIG

#include <tuner/Knob.h>

namespace tuner {

  // a data type that is suitable for use by mathematical models.
  // Conceptually, it is an ID-indexed snapshot of a KnobSet configuration.

  class KnobConfig {
  public:
    std::vector<std::pair<tuner::KnobID, int>> IntConfig;

  };

}

#endif // TUNER_KNOBCONFIG
