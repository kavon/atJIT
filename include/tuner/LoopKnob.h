#ifndef TUNER_LOOP_KNOBS
#define TUNER_LOOP_KNOBS

#include <tuner/Knob.h>

#include <optional>
#include <cinttypes>

namespace tuner {

  // https://llvm.org/docs/LangRef.html#llvm-loop
  struct LoopSetting {
    // std::option<bool> VectorizeEnable;
    // std::option<uint16_t> VectorizeWidth; // 1 == disable, 0 or omitted == "choose automatically"

    std::optional<bool> UnrollDisable;    // llvm.loop.unroll.disable
    std::optional<uint16_t> UnrollCount;  // llvm.loop.unroll.count
    std::optional<bool> UnrollFull;       // llvm.loop.unroll.full


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

    unsigned getLoopID() const { return LoopID; }

    void apply (llvm::Module &M) override;



  };


// handy type aliases.
namespace knob_type {
  using Loop = LoopKnob;
}

} // namespace tuner

std::ostream& operator<<(std::ostream &o, tuner::LoopSetting &LS);

#endif // TUNER_LOOP_KNOBS
