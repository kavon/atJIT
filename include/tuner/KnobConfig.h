#pragma once

#include <tuner/Knob.h>
#include <tuner/LoopKnob.h>
#include <tuner/KnobSet.h>
#include <tuner/Feedback.h>

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
    std::unordered_map<KnobID, int> IntConfig;
    std::unordered_map<KnobID, LoopSetting> LoopConfig;

  };

  template < typename RNE >  // meets the requirements of RandomNumberEngine
  KnobConfig genRandomConfig(KnobSet const &KS, RNE &Eng);

  extern template KnobConfig genRandomConfig<std::mt19937_64>(KnobSet const&, std::mt19937_64&);


  // energy = [0, 100], one can think of it as a "percentage of change".
  template < typename RNE >  // meets the requirements of RandomNumberEngine
  KnobConfig perturbConfig(KnobConfig KC, KnobSet const &KS, RNE &Eng, float energy);

  extern template KnobConfig perturbConfig<std::mt19937_64>(KnobConfig, KnobSet const &, std::mt19937_64 &, float);


  KnobConfig genDefaultConfig(KnobSet const&);

  void exportConfig(KnobConfig const& KC,
                    float* mat, const uint64_t row, const uint64_t ncol,
                    uint64_t const* colToKnob,
                    bool debug=false);

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

  ///////////
  // printing utils

  namespace {
    using T = std::pair<std::shared_ptr<KnobConfig>, std::shared_ptr<Feedback>>;
  }

  void dumpConfigInstance (std::ostream &os, KnobSet const& KS, T const &Entry);
  void dumpConfig (std::ostream &os, KnobSet const& KS, KnobConfig const &Config);

} // end namespace
