
#include <tuner/KnobConfig.h>

namespace tuner {

template < typename RNE >  // meets the requirements of RandomNumberEngine
KnobConfig genRandomConfig(KnobSet &KS, RNE &Eng) {
  KnobConfig Conf;

  for (auto Entry : KS.IntKnobs) {
    auto Knob = Entry.second;
    std::uniform_int_distribution<> dist(Knob->min(), Knob->max());
    Conf.IntConfig.push_back({Knob->getID(), dist(Eng)});
  }

  for (auto Entry : KS.LoopKnobs) {
    auto Knob = Entry.second;
    auto Setting = genRandomLoopSetting<RNE>(Eng);
    Conf.LoopConfig.push_back({Knob->getID(), Setting});
  }

  return Conf;
}

// specializations
template KnobConfig genRandomConfig<std::mt19937>(KnobSet&, std::mt19937&);

} // end namespace
