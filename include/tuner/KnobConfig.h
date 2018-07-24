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

  // TODO this should be just be a single vector<pair<KnobID, float>>,
  // since we need to be able to import & export configurations from
  // the model, which only deals with float. When applying a Config to
  // a KnobSet, the floats can be interpreted for what they mean.
  class KnobConfig {
  public:
    std::unordered_map<KnobID, int> IntConfig;
    std::unordered_map<KnobID, LoopSetting> LoopConfig;

  };

  template < typename RNE >  // meets the requirements of RandomNumberEngine
  KnobConfig genRandomConfig(KnobSet const &KS, RNE &Eng);

  extern template KnobConfig genRandomConfig<std::mt19937>(KnobSet const&, std::mt19937&);

  KnobConfig genDefaultConfig(KnobSet const&);

  void exportConfig(KnobConfig const& KC,
                    float* mat, const uint64_t row, const uint64_t ncol,
                    uint64_t const* colToKnob);

  class KnobConfigAppFn {
  public:
      virtual void operator()(std::pair<KnobID, int>) = 0;
      virtual void operator()(std::pair<KnobID, LoopSetting>) = 0;
  };

  // a version of the AppFn that only applies to the given knob ID.
  // uses lookups to find the Knob in the config. Is reusable.
  class KnobConfigSelFun : public KnobConfigAppFn {
    KnobID id_;
  public:
    KnobConfigSelFun(KnobID id) : id_(id) {}
    KnobID getID() const { return id_; }
    void setID(KnobID newID) { id_ = newID; }

    virtual void notFound() = 0;
  };

  void applyToConfig(KnobConfigAppFn &F, KnobConfig const &Settings);
  void applyToConfig(KnobIDAppFn &F, KnobConfig const &Settings);
  void applyToConfig(KnobConfigSelFun &F, KnobConfig const &Settings);

}

#endif // TUNER_KNOBCONFIG
