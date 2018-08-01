#ifndef TUNER_LOOP_KNOBS
#define TUNER_LOOP_KNOBS

#include <tuner/Knob.h>
#include <tuner/Util.h>

#include <optional>
#include <cinttypes>
#include <random>

namespace tuner {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

  namespace loop_md {
    static char const* TAG = "atJit.loop"; // make sure to keep this in sync with MDUtils.h

    static char const* UNROLL_DISABLE = "llvm.loop.unroll.disable";
    static char const* UNROLL_COUNT = "llvm.loop.unroll.count";
    static char const* UNROLL_FULL = "llvm.loop.unroll.full";
    static char const* VECTORIZE_ENABLE = "llvm.loop.vectorize.enable";
    static char const* VECTORIZE_WIDTH = "llvm.loop.vectorize.width";
    static char const* LICM_VER_DISABLE = "llvm.loop.licm_versioning.disable";
    static char const* INTERLEAVE_COUNT = "llvm.loop.interleave.count";
    static char const* DISTRIBUTE = "llvm.loop.distribute.enable";
  }

#pragma GCC diagnostic pop

  // https://llvm.org/docs/LangRef.html#llvm-loop
  //
  // NOTE if you add a new option here, make sure to update:
  // 1. LoopKnob.cpp::addToLoopMD
  //    1a. You might need to update MDUtils.h while doing this.
  // 2. operator<<(stream, LoopSetting)
  // 3. any tuners operating on a LoopSetting,
  //    like RandomTuner::genRandomLoopSetting
  //
  struct LoopSetting {
    // hints only
    std::optional<uint16_t> VectorizeWidth{}; // 1 = disable entirely, >= 2 suggests width

    // hint only
    std::optional<uint16_t> InterleaveCount{}; // 0 = off, 1 = automatic, >=2 is count.

    std::optional<bool> UnrollDisable{};    // llvm.loop.unroll.disable
    std::optional<bool> UnrollFull{};       // llvm.loop.unroll.full
    std::optional<uint16_t> UnrollCount{};  // llvm.loop.unroll.count

    std::optional<bool> LICMVerDisable{}; // llvm.loop.licm_versioning.disable

    std::optional<bool> Distribute{};

    size_t size() const {
      return 7;
    }

    static void flatten(float* slice, LoopSetting LS) {
      size_t i = 0;

      LoopSetting::flatten(slice + i++, LS.VectorizeWidth);

      LoopSetting::flatten(slice + i++, LS.InterleaveCount);

      LoopSetting::flatten(slice + i++, LS.UnrollDisable);
      LoopSetting::flatten(slice + i++, LS.UnrollFull);
      LoopSetting::flatten(slice + i++, LS.UnrollCount);

      LoopSetting::flatten(slice + i++, LS.LICMVerDisable);

      LoopSetting::flatten(slice + i++, LS.Distribute);


      if (i != LS.size())
        throw std::logic_error("size does not match expectations");
    }

    // TODO: is the use of MISSING here correct?

    static void flatten(float* slice, std::optional<bool> opt) {
      if (opt)
        *slice = opt.value() ? 1.0 : 0.0;
      else
        *slice = MISSING;
    }

    static void flatten(float* slice, std::optional<uint16_t> opt) {
      if (opt)
        *slice = (float) opt.value();
      else
        *slice = MISSING;
    }

  };

  class LoopKnob : public Knob<LoopSetting> {
  private:
    LoopSetting Opt;
    unsigned LoopID;

    // NOTE could probably add some utilities to check the
    // sanity of a loop setting to this class?


  public:
    LoopKnob (unsigned name) : LoopID(name) {}

    LoopSetting getDefault() const override {
      LoopSetting Empty;
      return Empty;
    }

    LoopSetting getVal() const override { return Opt; }

    void setVal (LoopSetting LS) override { Opt = LS; }

    unsigned getLoopName() const { return LoopID; }

    void apply (llvm::Module &M) override;

    virtual std::string getName() const override {
       return "loop #" + std::to_string(getLoopName());
    }

    virtual size_t size() const override {
      return Opt.size();
    }

  }; // end class


// any specializations of genRandomLoopSetting you would like to use
// should be declared as an extern template here, and then instantiated
// in LoopSettingGen, since I don't want to include the generic impl here.
// see: https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
template < typename RNE >  // meets the requirements of RandomNumberEngine
LoopSetting genRandomLoopSetting(RNE &Eng);

extern template
LoopSetting genRandomLoopSetting<std::mt19937_64>(std::mt19937_64&);


template < typename RNE >
LoopSetting genNearbyLoopSetting(RNE &Eng, LoopSetting LS, double energy);

extern template
LoopSetting genNearbyLoopSetting<std::mt19937_64>(std::mt19937_64&, LoopSetting, double);


// handy type aliases.
namespace knob_type {
  using Loop = LoopKnob;
}

} // namespace tuner

std::ostream& operator<<(std::ostream &o, tuner::LoopSetting &LS);

#endif // TUNER_LOOP_KNOBS
