
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
    Conf.IntConfig[Knob->getID()] = dist(Eng);
  }

  void operator()(std::pair<KnobID, knob_type::Loop*> I) override {
    auto Knob = I.second;
    auto Setting = genRandomLoopSetting<RNE>(Eng);
    Conf.LoopConfig[Knob->getID()] = Setting;
  }
}; // end class


class GenDefaultConfig : public KnobSetAppFn {
  KnobConfig Conf;
public:
  KnobConfig get() { return Conf; }

  #define HANDLE_CASE(KnobKind, Member)                                        \
  void operator()(std::pair<KnobID, KnobKind> I) override {                    \
    auto Knob = I.second;                                                      \
    Conf.Member[Knob->getID()] = Knob->getDefault();                           \
  }

  HANDLE_CASE(knob_type::Int*, IntConfig)
  HANDLE_CASE(knob_type::Loop*, LoopConfig)

  #undef HANDLE_CASE
}; // end class

class KnobConfigExporter : public KnobConfigSelFun {
private:
  float* slice_;
public:
  KnobConfigExporter() : KnobConfigSelFun(0), slice_(nullptr) {}
  void reset (KnobID newID, float* newSlice) {
    setID(newID);
    slice_ = newSlice;
  }

  virtual void operator()(std::pair<KnobID, int> I) override {
    *slice_ = (float)(I.second);
  }

  virtual void operator()(std::pair<KnobID, LoopSetting> I) override {
    LoopSetting::flatten(slice_, I.second);
  }

  virtual void notFound() override {
    // NOTE: if we knew the size of the slice, we could write MISSING
    // into it instead of bailing. we could get the size by simply
    // modifying the loop in exportConfig to trigger the export once it has
    // identified the end.
    printf("missing knob: %lu\n", getID());
    throw std::runtime_error("tried to export knob that doesn't exist in the config!");
  }

}; // end class


void exportConfig(KnobConfig const& KC,
                  float* mat, const uint64_t row, const uint64_t ncol,
                  uint64_t const* colToKnob) {
  KnobID prev = 0, cur = 0;
  KnobConfigExporter Exporter;

  for (uint64_t i = 0; i < ncol; prev = cur, ++i) {
    cur = colToKnob[i];
    if (prev == cur)
      continue;

    // we're at the starting column of knob 'cur'
    float* slice = mat + ((row * ncol) + i);
    Exporter.reset(cur, slice);
    applyToConfig(Exporter, KC);

    printf("exported knob %lu\n", cur);
  }

  // DEBUGGING
  printf("---\nrow %lu:\n", row);
  for (uint64_t i = 0; i < ncol; ++i) {
    printf("%f\n", mat[row * ncol + i]);
  }
  printf("---\n\n");

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

 void applyToConfig(KnobConfigSelFun &F, KnobConfig const &Settings) {
   #define LOOKUP(M)                                     \
   { auto Result = M.find(F.getID());                    \
     if (Result != M.end()) {                            \
       F(*Result);                                       \
       return;                                           \
     }                                                   \
   }

   LOOKUP(Settings.IntConfig)
   LOOKUP(Settings.LoopConfig)

   // if control reaches here, we couldn't find the knob!
   F.notFound();

   #undef LOOKUP
 }

} // end namespace
