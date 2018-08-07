#ifndef TUNER_BAYES_TUNER
#define TUNER_BAYES_TUNER

#include <tuner/AnalyzingTuner.h>
#include <tuner/Util.h>

#include <cstdlib>
#include <cassert>
#include <set>

#include <xgboost/c_api.h>

namespace tuner {

  namespace {
    class AssignToCols : public KnobSetAppFn {
      uint64_t i = 0;
      uint64_t* colToKnob;
    public:
      AssignToCols(uint64_t* fv1) : colToKnob(fv1) {}

      #define HANDLE_CASE(KnobKind)                                            \
      void operator()(std::pair<KnobID, KnobKind> I) override {                \
        auto Knob = I.second;                                                  \
        auto ID = I.first;                                                     \
        for (size_t k = 0; k < Knob->size(); ++k)                              \
          colToKnob[i++] = ID;                                                 \
      }

      HANDLE_CASE(knob_type::ScalarInt*)
      HANDLE_CASE(knob_type::Loop*)

      #undef HANDLE_CASE
    }; // end class
  } // end namespace

  // a tuner that uses techniques based on Bayesian Optimization
  // to find good configurations.
  class BayesianTuner : public RandomTuner {
  private:
    // the number of configs we have evaluated since we last trained a model
    uint32_t SinceLastTraining_ = 0;
    uint32_t BatchSz_ = 5;
    uint32_t SurrogateTestSz_ = 200;
    double Tradeoff_ = 0.5; // [0,1] indicates how much to "explore", with
                            // the remaining percent used to "exploit".

    std::list<KnobConfig> Predictions_; // current top predictions

    /////// members related to the dataset
    ////////////////////////////
    // cfg -- dense matrix where each row represents a knob configuration,
    //        and each column represents a knob. these are the measured inputs
    //        to our black-box function we're trying to model. the order of
    //        the rows corresponds to the order in Configs_
    //
    // result -- an array of length nrow containing training labels.
    //           result[i] is the observed output of configuration cfg[i].
    //
    // colToKnob -- a map from column numbers -> KnobIDs.
    //              a knob can span 1 or more contiguous columns, so col[i]
    //              may be the same as col[i+1];
    //
    uint64_t ncol = 0;
    uint64_t nrow = 0;
    float *result = NULL;
    uint64_t* colToKnob = NULL;

    // 'cfg' is actually a _dense_ 2D array of configuration data, i.e.,
    //       cfg[rowNum][i]  must be written as  cfg[(rowNum * ncol) + i]
    float *cfg = NULL;

    // updates the cfg and result matrices with new observations
    void updateDataset() {
      if (SinceLastTraining_ == 0)
        return;

      // resize the dataset as needed for new entries
      uint64_t prevNumRows = nrow;
      nrow += SinceLastTraining_;
      SinceLastTraining_ = 0;
      cfg = (float*) std::realloc(cfg, ncol * nrow * sizeof(float));
      result = (float*) std::realloc(result, nrow * sizeof(float));

      if (cfg == NULL || result == NULL)
        throw std::bad_alloc();

      // add the new observations to the dataset.
      for (uint64_t i = prevNumRows; i < nrow; ++i) {
        auto Entry = Configs_[i];
        auto KC = Entry.first;
        auto FB = Entry.second;

        exportConfig(*KC, cfg, i, ncol, colToKnob);

        auto measure = FB->avgMeasurement();
        // NOTE: i'm not sure if we're allowed to use MISSING here.
        if (!measure)
          throw std::logic_error("Bayes Tuner: feedback has no measured value");

        result[i] = measure.value();
      }
    }

    // returns true iff we succeeded in creating more predictions
    bool createPredictions() {
      ////////
      // First, we need to retrain a new model.
      ///////

      printf("STARTING TO SURF\n");

      updateDataset();

      // must be an array. not sure why.
      DMatrixHandle dmat[1];

      // add config data
      if (XGDMatrixCreateFromMat((float *) cfg, nrow, ncol, MISSING, &dmat[0]))
        throw std::runtime_error("XGDMatrixCreateFromMat failed.");

      // add labels, aka, outputs corresponding to configs
      if (XGDMatrixSetFloatInfo(dmat[0], "label", result, nrow))
        throw std::runtime_error("XGDMatrixSetFloatInfo failed.");

      // make a booster
      BoosterHandle booster;
      XGBoosterCreate(dmat, 1, &booster);
      // FIXME: all of these settings were picked arbitrarily
      XGBoosterSetParam(booster, "booster", "gbtree");
      XGBoosterSetParam(booster, "objective", "reg:linear");
      XGBoosterSetParam(booster, "max_depth", "5");
      XGBoosterSetParam(booster, "eta", "0.3");
      XGBoosterSetParam(booster, "min_child_weight", "1");
      XGBoosterSetParam(booster, "subsample", "0.5");
      XGBoosterSetParam(booster, "colsample_bytree", "1");
      XGBoosterSetParam(booster, "num_parallel_tree", "4");

      // learn
      for (int i = 0; i < 500; i++) {
        // FIXME: I just picked an arbitrary # of iters.
        // Using cross-validation to train up to some accuracy level
        // would be smarter.
        XGBoosterUpdateOneIter(booster, i, dmat[0]);
      }

      ////////
      // Next, we use the model to predict the running time of configurations
      // in the pool.
      ///////

      // build test cases
      std::vector<KnobConfig> Test;
      float* testMat = (float*) malloc(SurrogateTestSz_ * ncol * sizeof(float));
      uint32_t ExploreSz = std::round(SurrogateTestSz_ * Tradeoff_);
      uint32_t XPloitSz = SurrogateTestSz_ - ExploreSz;
      uint32_t rowNum = 0;

      {
        // cases that are used for exploration
        for (uint32_t i = 0; i < ExploreSz; ++i, ++rowNum) {
          KnobConfig KC = genRandomConfig(KS_, Gen_);
          exportConfig(KC, testMat, rowNum, ncol, colToKnob);
          Test.push_back(KC);
        }
      }

      {
        // cases that exploit the best case we've seen so far.
        auto Best = bestSeen();
        KnobConfig BestKC;
        if (Best.has_value())
          BestKC = *Best.value().first;
        else
          BestKC = genDefaultConfig(KS_);

        for (uint32_t i = 0; i < XPloitSz; ++i, ++rowNum) {
          // FIXME: right now I just picked an arbitrary energy level.
          KnobConfig KC = perturbConfig(BestKC, KS_, Gen_, 30.0);
          exportConfig(KC, testMat, rowNum, ncol, colToKnob);
          Test.push_back(KC);
        }
      }


      // get predictions
      DMatrixHandle h_test;
      XGDMatrixCreateFromMat((float *) testMat, SurrogateTestSz_, ncol, MISSING, &h_test);
      bst_ulong out_len;
      const float *out;
      XGBoosterPredict(booster, h_test, 0, 0, &out_len, &out);


      // pick best configs based on predictions ones
      using SetKey = std::pair<uint32_t, float>;

      struct lessThan {
        constexpr bool operator()(const SetKey &lhs, const SetKey &rhs) const
        {
            return lhs.second < rhs.second;
        }
      };

      std::multiset<SetKey, lessThan> Best;
      for (uint32_t i = 0; i < out_len; i++) {
        // DEBUG
        std::cout << "prediction[" << i << "]=" << out[i] << std::endl;
        auto Cur = std::make_pair(i, out[i]);

        if (Best.size() < BatchSz_) {
          Best.insert(Cur);
          continue;
        }

        auto UB = Best.upper_bound(Cur);
        if (UB != Best.end()) {
          // then UB is greater than Cur
          Best.erase(UB);
          Best.insert(Cur);
        }
      }

      for (auto Entry : Best) {
        auto Chosen = Entry.first;
        std::cout << "chose config " << Chosen
                  << " with estimated time " << Entry.second << std::endl;
        Predictions_.push_back(Test[Chosen]);
      }

      // DONE

      // free memory related to the XGB objects.
      XGDMatrixFree(dmat[0]);
      XGBoosterFree(booster);
      XGDMatrixFree(h_test);
      free(testMat);

      return (Predictions_.size() > 0);
    }


    GenResult& saveConfig(KnobConfig KC) {
      auto Conf = std::make_shared<KnobConfig>(KC);
      auto FB = std::make_shared<ExecutionTime>(Cxt_->getFeedbackStdErr());

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      SinceLastTraining_++;

      return Configs_.back();
    }

  ///////////////// PUBLIC //////////////////
  public:
    BayesianTuner(KnobSet KS, std::shared_ptr<easy::Context> Cxt) : RandomTuner(KS, std::move(Cxt)) {
    }

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~BayesianTuner() {
      std::free(cfg);
      std::free(result);
      std::free(colToKnob);
    }

    void analyze(llvm::Module &M) override {
      RandomTuner::analyze(M);

      // now that the number of knobs will no longer change,
      // we can setup part of the dataset

      if (colToKnob == NULL) {
        ncol = KS_.size();
        colToKnob = (uint64_t*) std::malloc(ncol * sizeof(uint64_t));
        assert(colToKnob && "bad alloc!");

        AssignToCols F(colToKnob);
        applyToKnobs(F, KS_);
      }
    }

    // either we're still establishing a random prior,
    // or we have a bounded set of predictions still sitting around.
    bool shouldCompileNext () override {
      size_t numConfg = Configs_.size();
      return numConfg < BatchSz_  ||  Predictions_.size() > 0;
    }

    GenResult& getNextConfig() override {
      // SURF
      size_t numConfg = Configs_.size();

      if (numConfg < BatchSz_) {
        // we're still at Step 2, trying to establish a prior
        // so we sample completely randomly

        // NOTE: I think it's useful to always include the default config
        // in the first batch.
        if (numConfg == 0)
          return saveConfig(genDefaultConfig(KS_));

        return saveConfig(genRandomConfig(KS_, Gen_));
      }

      // if we're out of predictions, retrain and produce new best-configs.
      if (Predictions_.empty() && !createPredictions())
        throw std::logic_error("Bayes Tuner: createPredictions failed");

      // return the next-best config.
      auto KC = Predictions_.front();
      Predictions_.pop_front();
      return saveConfig(KC);
    }

  }; // end class BayesianTuner

} // namespace tuner

#endif // TUNER_BAYES_TUNER
