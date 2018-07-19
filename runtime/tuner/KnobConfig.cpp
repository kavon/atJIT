
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
}; // end class


void exportConfig(KnobConfig const& KC,
                  float* mat, uint64_t row, uint64_t ncol,
                  uint64_t* colToKnob, const float MISSING) {
  // NOTE: we cannot properly implement this yet, because of a mistake
  // in how we generate knobs: right now we only have one
  // knob _per_ loop, and that knob consists of a large number of settings.
  // we need to flatten that out to one knob per setting, per loop.

  // throw std::runtime_error("implement me!");
  for (uint64_t i = 0; i < ncol; ++i)
    mat[(row * ncol) + i] = 3.0;
}

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

 void applyToConfig(KnobIDAppFn &F, KnobConfig const &Settings) {
   for (auto V : Settings.IntConfig) F(V.first);
   for (auto V : Settings.LoopConfig) F(V.first);
 }

} // end namespace
