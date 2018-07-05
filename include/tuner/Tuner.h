#ifndef TUNER_TUNER
#define TUNER_TUNER

#include <tuner/Feedback.h>
#include <tuner/KnobConfig.h>
#include <tuner/KnobSet.h>

#include <llvm/IR/Module.h>

#include <iostream>

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

    virtual void analyze(llvm::Module &M) = 0;

    // applies a configuration to the knobs managed by this tuner.
    void applyConfig (KnobConfig const &Config) {
      for (auto Entry : Config.IntConfig) {
        auto ID = Entry.first;
        auto Val = Entry.second;

        auto Knob = KS_.IntKnobs[ID];
        Knob->setVal(Val);
      }
    }

    /////////
    // utilites

    void dumpConfig (KnobConfig const &Config) const {
      std::cout << "{\n";
      for (auto Entry : Config.IntConfig) {
        auto ID = Entry.first;
        auto Val = Entry.second;

        auto search = KS_.IntKnobs.find(ID);
        if (search == KS_.IntKnobs.end()) {
          std::cout << "dumpConfig ERROR: a knob in the given Config "
                    << "is not in Tuner's KnobSet!\n";

          throw std::runtime_error("unknown knob ID");
        }

        auto Knob = search->second;
        std::cout << Knob->getName() << " := " << Val << "\n";
      }
      std::cout << "}\n";
    }

    void dumpConfigInstance (GenResult const &Entry) const {
      auto Conf = Entry.first;
      auto FB = Entry.second;
      FB->dump();
      dumpConfig(*Conf);
      std::cout << "\n";
    }

    void dump() const {
      std::cout << "-----------\n";
      for (auto Entry : Configs_) {
        dumpConfigInstance(Entry);
      }
      std::cout << "-----------\n";
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

    GenResult& getNextConfig () override {
      return NoOpConfig_;
    }

    void analyze(llvm::Module &M) override { }
  };

} // namespace tuner

#endif // TUNER_TUNER
