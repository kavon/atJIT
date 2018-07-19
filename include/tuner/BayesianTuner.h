#ifndef TUNER_BAYES_TUNER
#define TUNER_BAYES_TUNER

#include <tuner/AnalyzingTuner.h>

#include <cstdlib>
#include <cassert>

#include <xgboost/c_api.h>

namespace tuner {

  namespace {
    class AssignToCols : public KnobIDAppFn {
      uint64_t i = 0;
      uint64_t* colToKnob;
    public:
      AssignToCols(uint64_t* fv1) : colToKnob(fv1) {}
      void operator()(KnobID id) {
        colToKnob[i++] = id;
      }
    }; // end class
  } // end namespace

  // a tuner that uses techniques based on Bayesian Optimization
  // to find good configurations.
  class BayesianTuner : public AnalyzingTuner {
  private:
    // the number of configs we have evaluated since we last trained a model
    uint32_t SinceLastTraining_ = 0;
    uint32_t BatchSz_ = 32;

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
    // colToKnob -- a map from KnobIDs -> column numbers, implemented as an array.
    //
    static constexpr float MISSING = std::numeric_limits<float>::quiet_NaN();
    uint64_t ncol = 0;
    uint64_t nrow = 0;
    float *result = NULL;
    uint64_t* colToKnob = NULL;

    // 'cfg' is actually a _dense_ 2D array of configuration data, i.e.,
    //       cfg[rowNum][i]  must be written as  cfg[(rowNum * ncol) + i]
    float *cfg = NULL;

    std::mt19937 RNE_; // // 32-bit mersenne twister random number generator


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

        exportConfig(*KC, cfg, i, ncol, colToKnob, MISSING);

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

      updateDataset();

      // must be a array. not sure why.
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
      XGBoosterSetParam(booster, "booster", "gbtree");
      XGBoosterSetParam(booster, "objective", "reg:linear");
      XGBoosterSetParam(booster, "max_depth", "5");
      XGBoosterSetParam(booster, "eta", "0.1");
      XGBoosterSetParam(booster, "min_child_weight", "1");
      XGBoosterSetParam(booster, "subsample", "0.5");
      XGBoosterSetParam(booster, "colsample_bytree", "1");
      XGBoosterSetParam(booster, "num_parallel_tree", "1");

      // learn
      for (int i = 0; i < 200; i++) {
        XGBoosterUpdateOneIter(booster, i, dmat[0]);
      }

      ////////
      // Next, we use the model to predict the running time of configurations
      // in the pool.
      ///////

      // TODO generate configurations, turn them into a DMatrix,
      // and ask the model to predict the times.

      // TODO pick the top BatchSz performers from the prediction,
      // and push them onto the Predictions queue.

      // TODO free memory related to the XGB objects.

      return false;
    }


    GenResult& saveConfig(KnobConfig KC) {
      auto Conf = std::make_shared<KnobConfig>(KC);
      auto FB = std::make_shared<ExecutionTime>();

      // keep track of this config.
      Configs_.push_back({Conf, FB});
      SinceLastTraining_++;

      return Configs_.back();
    }

  ///////////////// PUBLIC //////////////////
  public:
    BayesianTuner(KnobSet KS) : AnalyzingTuner(KS) {
      unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
      RNE_ = std::mt19937(seed);
    }

    // we do not free any knobs, since MPM or other objects
    // should end up freeing them.
    ~BayesianTuner() {
      std::free(cfg);
      std::free(result);
      std::free(colToKnob);
    }

    void analyze(llvm::Module &M) override {
      AnalyzingTuner::analyze(M);

      // now that the number of knobs will no longer change,
      // we can setup part of the dataset

      ncol = KS_.size();
      colToKnob = (uint64_t*) std::malloc(ncol * sizeof(uint64_t));
      assert(colToKnob && "bad alloc!");

      AssignToCols F(colToKnob);
      applyToKnobs(F, KS_);
    }

    GenResult& getNextConfig() override {
      // SURF

      if (Configs_.size() < BatchSz_) {
        // we're still at Step 2, trying to establish a prior
        // so we sample randomly
        return saveConfig(genRandomConfig(KS_, RNE_));
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
