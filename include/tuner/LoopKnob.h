#ifndef TUNER_LOOP_KNOBS
#define TUNER_LOOP_KNOBS

#include <tuner/Knob.h>

#include <optional>
#include <cinttypes>

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
  }

#pragma GCC diagnostic pop

  // https://llvm.org/docs/LangRef.html#llvm-loop
  //
  // NOTE if you add a new option here, make sure to update:
  // 1. LoopKnob.cpp::addToLoopMD
  //    1a. You'll want to update MDUtils.h while doing this.
  // 2. operator<<(stream, LoopSetting)
  // 3. any tuners operating on a LoopSetting,
  //    like RandomTuner::genRandomLoopSetting
  //
  struct LoopSetting {
    std::optional<bool> VectorizeEnable{};
    std::optional<uint16_t> VectorizeWidth{}; // >= 2, omitted == "choose automatically"

    std::optional<bool> UnrollDisable{};    // llvm.loop.unroll.disable
    std::optional<bool> UnrollFull{};       // llvm.loop.unroll.full
    std::optional<uint16_t> UnrollCount{};  // llvm.loop.unroll.count

    std::optional<bool> LICMVerDisable{}; // llvm.loop.licm_versioning.disable

  };

  class LoopKnob : public Knob<LoopSetting> {
  private:
    LoopSetting Opt;
    unsigned LoopID;

    // NOTE could probably add some utilities to check the
    // sanity of a loop setting to this class?


  public:
    LoopKnob (unsigned name) : LoopID(name) {}

    LoopSetting getVal() const override { return Opt; }

    void setVal (LoopSetting LS) override { Opt = LS; }

    unsigned getLoopName() const { return LoopID; }

    void apply (llvm::Module &M) override;

    virtual std::string getName() const override {
       return "loop #" + std::to_string(getLoopName());
    }

  };


// handy type aliases.
namespace knob_type {
  using Loop = LoopKnob;
}

} // namespace tuner

std::ostream& operator<<(std::ostream &o, tuner::LoopSetting &LS);

#endif // TUNER_LOOP_KNOBS
