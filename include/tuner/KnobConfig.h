#ifndef TUNER_KNOBCONFIG
#define TUNER_KNOBCONFIG

#include <tuner/Knob.h>
#include <tuner/LoopKnob.h>

#include <vector>
#include <utility>

namespace tuner {

  // a data type that is suitable for use by mathematical models.
  // Conceptually, it is an ID-indexed snapshot of a KnobSet configuration.

  class KnobConfig {
  public:
    std::vector<std::pair<KnobID, int>> IntConfig;
    std::vector<std::pair<KnobID, LoopSetting>> LoopConfig;

  };

}

#endif // TUNER_KNOBCONFIG
