#ifndef TUNER_TUNER
#define TUNER_TUNER

#include <tuner/Feedback.h>
#include <tuner/KnobConfig.h>
#include <tuner/KnobSet.h>

#include <llvm/IR/Module.h>

#include <algorithm>
#include <cfloat>
#include <iostream>

namespace tuner {

  using GenResult = std::pair<std::shared_ptr<KnobConfig>, std::shared_ptr<Feedback>>;

  // generic operations over KnobSet/KnobConfig components.
  namespace {
    class KnobAdjustor : public KnobConfigAppFn {
      KnobSet &KS;
      llvm::Module &M;

    public:
      KnobAdjustor(KnobSet &KSet, llvm::Module &Mod) : KS(KSet), M(Mod) {}

      #define HANDLE_CASE(ValTy, KnobMap)                                      \
        void operator()(std::pair<KnobID, ValTy> Entry) override {             \
          auto ID = Entry.first;                                               \
          auto NewVal = Entry.second;                                          \
          auto Knobs = KS.KnobMap;                                             \
          auto TheKnob = Knobs[ID];                                            \
          TheKnob->setVal(NewVal);                                             \
          TheKnob->apply(M);                                                   \
        }

      HANDLE_CASE(int, IntKnobs)
      HANDLE_CASE(LoopSetting, LoopKnobs)

      #undef HANDLE_CASE

    }; // end class

    class ConfigDumper : public KnobConfigAppFn {
      KnobSet const &KS;
    public:
      ConfigDumper(KnobSet const &KSet) : KS(KSet) {}

      #define HANDLE_CASE(ValTy, KnobMap)                                      \
      void operator()(std::pair<KnobID, ValTy> Entry) override {               \
        auto ID = Entry.first;                                                 \
        auto Val = Entry.second;                                               \
                                                                               \
        auto search = KS.KnobMap.find(ID);                                     \
        if (search == KS.KnobMap.end()) {                                      \
          std::cout << "dumpConfig ERROR: a knob in the given Config "         \
                    << "is not in Tuner's KnobSet!\n";                         \
                                                                               \
          throw std::runtime_error("unknown knob ID");                         \
        }                                                                      \
                                                                               \
        auto Knob = search->second;                                            \
        std::cout << Knob->getName() << " := " << Val << "\n";                 \
      }

      HANDLE_CASE(int, IntKnobs)
      HANDLE_CASE(LoopSetting, LoopKnobs)

      #undef HANDLE_CASE

    }; // end class

  } // end anonymous namespace

  struct {
    bool operator()(GenResult const &A, GenResult const &B) const
    { // smallest time last. if no time is available, it goes to the front.
        std::optional<double> timeA = A.second->avgMeasurement();
        std::optional<double> timeB = B.second->avgMeasurement();

        return timeA.value_or(DBL_MAX) >= timeB.value_or(DBL_MAX);
    }
  } resultGEQ;

  class Tuner {
  protected:

    KnobSet KS_;
    std::vector<GenResult> Configs_;

  public:

    Tuner(KnobSet KS) : KS_(KS) {}

    // ensures derived destructors are called
    virtual ~Tuner() = default;

    // this method should not mutate the value of any Knobs,
    // it _may_ mutate the state of the Tuner.
    // NOTE: NOT THREAD SAFE.
    virtual GenResult& getNextConfig () = 0;

    // a method to query whether the tuner
    // could produce a new config, given no
    // additional measurement feedback information
    // from prior configs. This is used to allow more
    // compilation tasks to occur concurrently.
    // NOTE: NOT THREAD SAFE
    virtual bool nextConfigPossible() const = 0;

    virtual void analyze(llvm::Module &M) = 0;

    // applies a configuration to the given LLVM module via the
    // knobs managed by this tuner.
    void applyConfig (KnobConfig const &Config, llvm::Module &M) {
      KnobAdjustor F(KS_, M);
      applyToConfig(F, Config);
    }

    /////////
    // utilites

    void dumpConfig (KnobConfig const &Config) const {
      std::cout << "{\n";
      ConfigDumper F(KS_);
      applyToConfig(F, Config);
      std::cout << "}\n";
    }

    void dumpConfigInstance (GenResult const &Entry) const {
      auto Conf = Entry.first;
      auto FB = Entry.second;
      FB->dump();
      dumpConfig(*Conf);
      std::cout << "\n";
    }

    virtual void dump() {


      std::sort(Configs_.begin(), Configs_.end(), resultGEQ);

      std::cout << "-----------\n";
      for (auto Entry : Configs_) {
        dumpConfigInstance(Entry);
      }
      std::cout << "-----------\n";
    }

    std::optional<GenResult> bestSeen() {
      std::optional<GenResult> best = std::nullopt;

      for (auto Entry : Configs_) {
        if (best.has_value() == false || resultGEQ(best.value(), Entry))
          best = Entry;
      }

      return best;
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

    bool nextConfigPossible () const override {
      return true;
    }

    void analyze(llvm::Module &M) override { }
  };

} // namespace tuner

#endif // TUNER_TUNER
