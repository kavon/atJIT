#ifndef TUNER_ANNEALING_TUNER
#define TUNER_ANNEALING_TUNER

#include <tuner/RandomTuner.h>

#include <cmath>

namespace tuner {

  //////////////
  // a tuner that uses Simulated Annealing. The algorithm is based on
  // the description in:
  //
  // Dimitris Bertsimas and John Tsitsiklis. "Simulated annealing."
  // Statistical Science 8, no. 1 (1993): 10-15.
  //
  class AnnealingTuner : public RandomTuner {
    uint64_t timeStep;
    const double EscapeDifficulty = 50000;  // corresponds to d*
    const double MaxEnergy = 100.0;
    double MaxTemp; // determined by cooling schedule

    bool initalizedFirstState = false;

    GenResult currentState;
    GenResult trialState;
  private:
    GenResult& saveConfig(KnobConfig KC) {
      auto Conf = std::make_shared<KnobConfig>(KC);
      auto FB = std::make_shared<ExecutionTime>(Cxt_->getFeedbackStdErr());

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      return Configs_.back();
    }

    bool missingCost(GenResult const& R) const {
      return R.second->avgMeasurement().has_value() == false;
    }

    double getCost(GenResult const& R) const {
      return R.second->avgMeasurement().value();
    }

    // corresponds to T(t)
    double coolingSchedule(uint64_t step) const {
      // log(0) == -inf => T(0) ~= 0
      // log(1) == 0    => T(1) ~= inf
      assert(step > 1 && "unexpected time step");
      return EscapeDifficulty / std::log(step);
    }

    // the amount of "energy" that should be used to perturb the config.
    double perturbEnergy() {
      if (timeStep < 2)
        return MaxEnergy;

      // scale based how cool the system is.
      return (coolingSchedule(timeStep) / MaxTemp) * MaxEnergy;
    }

    // choose the new current state given that we are trying to minimize the
    // "cost", aka, running time.
    GenResult chooseNextState() {
      // when computing a probability, we know trial > cur, so diff > 0.
      // thus, for the initial time steps, which are weird, probabilities are:
      //
      // T(0) => P(trial) ~= exp(-diff / 0)   ~= exp(-inf) ~= 0
      // T(1) => P(trial) ~= exp(-diff / inf) ~= exp(-0)   ~= 1

      double cur = getCost(currentState);
      double trial = getCost(trialState);

      if (trial <= cur)
        return trialState;

      // determine the probability of choosing the trial state.
      double prob;

      // handle special edge cases
      if (timeStep == 0)
        prob = 0.0;
      else if (timeStep == 1)
        prob = 1.0;
      else
        prob = std::exp(-( (trial - cur) / coolingSchedule(timeStep) ));

      assert(prob >= 0.0 && prob <= 1.0);

      // make the decision
      std::uniform_real_distribution<> dis(0, 1); // [0, 1)
      if (dis(Gen_) >= prob)
        return currentState;

      return trialState;
    }


  public:
    AnnealingTuner(KnobSet KS, std::shared_ptr<easy::Context> Cxt)
      : RandomTuner(KS, std::move(Cxt)) {
        timeStep = 0;
        MaxTemp = coolingSchedule(2); // largest value the schedule takes on.
      }

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~AnnealingTuner() {}

    void analyze(llvm::Module &M) override {
      RandomTuner::analyze(M);

      // we do this here instead of in the constructor because we
      // want the first start to be aware of _all_ knobs.
      if (!initalizedFirstState) {
        currentState = saveConfig(genDefaultConfig(KS_));
        trialState = saveConfig(genRandomConfig(KS_, Gen_));
        initalizedFirstState = true;
      }
    }

    bool nextConfigPossible () const override {
      return initalizedFirstState &&
        (!(missingCost(currentState)) || missingCost(trialState));
    }

    GenResult& getNextConfig() override {
      // ensure we have a cost for the currentState
      if (missingCost(currentState))
        return currentState;

      // ... and a cost for the trialState
      if (missingCost(trialState))
        return trialState;

      // now we can determine the next state.
      currentState = chooseNextState();
      timeStep++;

      // produce a trial state, which is a neighbor of the new current.
      double energy = perturbEnergy();
      auto curConf = *currentState.first;
      trialState = saveConfig(perturbConfig(curConf, KS_, Gen_, energy));

      return trialState;
    }

    void dump() override {
      if (timeStep > 1)
        std::cout << "step: " << timeStep
                  << ", temperature: " <<  coolingSchedule(timeStep)
                  << ", energy: " << perturbEnergy()
                  << std::endl;

      if (auto best = bestSeen()) {
        std::cout << "------- best config ----------\n";
        dumpConfigInstance(best.value());
      }

      std::cout << "------- current config --------\n";
      dumpConfigInstance(currentState);

      std::cout << "------- trial config --------\n";
      dumpConfigInstance(trialState);
    }

  }; // end class AnnealingTuner

} // namespace tuner

#endif // TUNER_ANNEALING_TUNER
