#ifndef TUNER_KNOBCONFIG
#define TUNER_KNOBCONFIG

#include <tuner/Knob.h>
#include <tuner/LoopKnob.h>
#include <tuner/KnobSet.h>

#include <vector>
#include <utility>

namespace tuner {

  // a data type that is suitable for use by mathematical models.
  // Conceptually, it is an ID-indexed snapshot of a KnobSet configuration.

  // NOTE if you add another structure member, you must immediately update:
  //
  // 0. class KnobSet
  // 1. KnobConfigAppFn and related abstract visitors.
  // 2. applyToConfig and related generic operations.

  class KnobConfig {
  public:
    std::vector<std::pair<KnobID, int>> IntConfig;
    std::vector<std::pair<KnobID, LoopSetting>> LoopConfig;

  };

  template < typename RNE >  // meets the requirements of RandomNumberEngine
  KnobConfig genRandomConfig(KnobSet const &KS, RNE &Eng);

  extern template KnobConfig genRandomConfig<std::mt19937>(KnobSet const&, std::mt19937&);

  KnobConfig genDefaultConfig(KnobSet const&);

  class KnobConfigAppFn {
  public:
      virtual void operator()(std::pair<KnobID, int>) = 0;
      virtual void operator()(std::pair<KnobID, LoopSetting>) = 0;
  };

  void applyToConfig(KnobConfigAppFn &F, KnobConfig const &Settings);

}

#endif // TUNER_KNOBCONFIG
