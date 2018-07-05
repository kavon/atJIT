#ifndef TUNER_TUNER
#define TUNER_TUNER

#include <tuner/Feedback.h>
#include <tuner/KnobConfig.h>
#include <tuner/KnobSet.h>

namespace tuner {

  class Tuner {
  protected:
    using GenResult = std::pair<std::shared_ptr<KnobConfig>, std::shared_ptr<Feedback>>;

    KnobSet KS_;
    std::vector<GenResult> Configs_;

  public:

    Tuner(KnobSet KS) : KS_(KS) {}

    // ensures derived destructors are called
    virtual ~Tuner() = default;

    // NOTE: this method should not mutate the value of any Knobs,
    // it _may_ mutate the state of the Tuner.
    virtual GenResult& getNextConfig () = 0;

    // applies a configuration to the knobs managed by this tuner.
    void applyConfig (KnobConfig const &Config) {
      for (auto Entry : Config.IntConfig) {
        auto ID = Entry.first;
        auto Val = Entry.second;

        auto Knob = KS_.IntKnobs[ID];
        Knob->setVal(Val);
      }

    }

  }; // end class Tuner

  class NoOpTuner : public Tuner {
  private:
    GenResult NoOpConfig_;
  public:

    NoOpTuner(KnobSet KS) : Tuner(KS) {
      NoOpConfig_ = { std::make_shared<KnobConfig>(),
                      std::make_shared<NoOpFeedback>() };
    }

    virtual GenResult& getNextConfig () override {
      return NoOpConfig_;
    }
  };

} // namespace tuner

#endif // TUNER_TUNER
