
#include <tuner/KnobConfig.h>

namespace tuner {

template < typename RNE >
class GenSimpleRandConfig : public KnobSetAppFn {
  RNE &Eng;
  KnobConfig Conf;
public:
  GenSimpleRandConfig (RNE &Engine) : Eng(Engine) {}
  KnobConfig get() { return Conf; }

  void operator()(std::pair<KnobID, knob_type::Int*> I) override {
    auto Knob = I.second;
    std::uniform_int_distribution<> dist(Knob->min(), Knob->max());
    Conf.IntConfig.push_back({Knob->getID(), dist(Eng)});
  }

  void operator()(std::pair<KnobID, knob_type::Loop*> I) override {
    auto Knob = I.second;
    auto Setting = genRandomLoopSetting<RNE>(Eng);
    Conf.LoopConfig.push_back({Knob->getID(), Setting});
  }
}; // end class


class GenDefaultConfig : public KnobSetAppFn {
  KnobConfig Conf;
public:
  KnobConfig get() { return Conf; }

  #define HANDLE_CASE(KnobKind, Member)                                        \
  void operator()(std::pair<KnobID, KnobKind> I) override {                    \
    auto Knob = I.second;                                                      \
    Conf.Member.push_back({Knob->getID(), Knob->getDefault()});                \
  }

  HANDLE_CASE(knob_type::Int*, IntConfig)
  HANDLE_CASE(knob_type::Loop*, LoopConfig)

  #undef HANDLE_CASE
}; // end case

KnobConfig genDefaultConfig(KnobSet const& KS) {
  GenDefaultConfig F;
  applyToKnobs(F, KS);
  return F.get();
}

/////////////////////////
template < typename RNE >  // meets the requirements of RandomNumberEngine
KnobConfig genRandomConfig(KnobSet const &KS, RNE &Eng) {
  GenSimpleRandConfig<RNE> F(Eng);
  applyToKnobs(F, KS);
  return F.get();
}

// specializations
template KnobConfig genRandomConfig<std::mt19937>(KnobSet const&, std::mt19937&);

////////////////////////////

void applyToConfig(KnobConfigAppFn &F, KnobConfig const &Settings) {
  for (auto V : Settings.IntConfig) F(V);
  for (auto V : Settings.LoopConfig) F(V);
}

} // end namespace
