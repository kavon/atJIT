#pragma once

#include <tuner/AnalyzingTuner.h>
#include <tuner/Util.h>

#include <loguru.hpp>

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
    const uint32_t BatchSz_ = 5;
    const uint32_t SurrogateTestSz_ = 200;
    const unsigned MaxLearnIters_ = 500;
    const unsigned MaxLearnPastBest_ = 0;
    const double HeldoutRatio_ = 0.1; // training set vs validation set ratio.
    const double Tradeoff_ = 0.5; // [0,1] indicates how much to "explore", with
                            // the remaining percent used to "exploit".

    int aheadOfTimeCount = 0;

    std::list<KnobConfig> Predictions_; // current top predictions

    /////// members related to the dataset
    //////////////////////////// FIXME: all of these comments are out of date now
    // cfg -- dense matrix where each row represents a knob configuration,
    //        and each column represents a knob. these are the measured inputs
    //        to our black-box function we're trying to model. the order of
    //        the rows corresponds to the order in Configs_
    //
    // result -- an array of length trainingRows containing training labels.
    //           result[i] is the observed output of configuration cfg[i].
    //
    // colToKnob -- a map from column numbers -> KnobIDs.
    //              a knob can span 1 or more contiguous columns, so col[i]
    //              may be the same as col[i+1];
    //
    uint64_t ncol = 0;
    uint64_t* colToKnob = NULL;

    // 'cfg' is actually a _dense_ 2D array of configuration data, i.e.,
    //       cfg[rowNum][i]  must be written as  cfg[(rowNum * ncol) + i]
    // this data is used for TRAINING
    // each row corrsponds to one configuration.
    float *cfg = NULL;
    float *result = NULL;
    uint64_t trainingRows = 0;

    // same as cfg, but these observations are held-out for cross-validation
    float *test = NULL;
    float *testResult = NULL;
    uint64_t validateRows = 0;

    void initBooster(const DMatrixHandle dmats[],
                      bst_ulong len,
                      BoosterHandle *out) {

      XGBoosterCreate(dmats, len, out);
      // NOTE: all of these settings were picked arbitrarily
#ifdef NDEBUG
      XGBoosterSetParam(*out, "silent", "1");
#endif
      XGBoosterSetParam(*out, "booster", "gbtree");
      XGBoosterSetParam(*out, "objective", "reg:linear");
      XGBoosterSetParam(*out, "max_depth", "5");
      XGBoosterSetParam(*out, "eta", "0.3");
      XGBoosterSetParam(*out, "min_child_weight", "1");
      XGBoosterSetParam(*out, "subsample", "0.5");
      XGBoosterSetParam(*out, "colsample_bytree", "1");
      XGBoosterSetParam(*out, "num_parallel_tree", "4");
    }

    // updates the train, and result matrices with new observations
    void updateDataset() {
      if (SinceLastTraining_ == 0)
        return;
      SinceLastTraining_ = 0;

      // reconstruct held-out set and training set
      const uint64_t numConfigs = Configs_.size();
      validateRows = std::max(1, (int) std::round(HeldoutRatio_ * numConfigs));
      trainingRows = numConfigs - validateRows;

      const size_t rowSize = ncol * sizeof(float);

      std::free(cfg);
      std::free(result);
      std::free(test);
      std::free(testResult);

      cfg = (float*) std::malloc(rowSize * trainingRows);
      result = (float*) std::malloc(sizeof(float) * trainingRows);

      test = (float*) std::malloc(rowSize * validateRows);
      testResult = (float*) std::malloc(sizeof(float) * validateRows);

      if (cfg == NULL || result == NULL || test == NULL || testResult == NULL)
        throw std::bad_alloc();

      // shuffle the data
      std::random_shuffle(Configs_.begin(), Configs_.end());

      // the first few will be part of the held-out set, and the
      // rest go to the training set.
      float* configOut = test;
      float* resultOut = testResult;
      size_t i = 0; // index into the current matrix's row
      for (size_t count = 0; count < numConfigs; count++, i++) {
        if (count == validateRows) {
          // reset, the rest go to the training set
          configOut = cfg;
          resultOut = result;
          i = 0;
        }

        auto Entry = Configs_[count];
        auto KC = Entry.first;
        auto FB = Entry.second;

        exportConfig(*KC, configOut, i, ncol, colToKnob);

        FB->updateStats();
        double time = FB->expectedValue();

        if (time == std::numeric_limits<double>::max())
          ABORT_F("Bayes Tuner -- training on unevaluated code fragment!");

        resultOut[i] = time;
      }

      assert(i == trainingRows && "bad generation of dataset");
    }




    // returns true iff we succeeded in creating more predictions
    bool createPredictions() {
      ////////
      // First, we need to retrain a new model.
      ///////

#ifndef NDEBUG
      std::cout << "Bayesian Tuner is making predictions...\n";
#endif

      updateDataset();

#ifndef NDEBUG
        std::cout << trainingRows << " training observations, "
                  << validateRows << " observations held out.\n";
#endif

      // must be an array. not sure why.
      DMatrixHandle train[1];

      // add config data
      if (XGDMatrixCreateFromMat((float *) cfg, trainingRows, ncol, MISSING, &train[0]))
        throw std::runtime_error("XGDMatrixCreateFromMat failed.");

      // add labels, aka, outputs corresponding to datasets
      if (XGDMatrixSetFloatInfo(train[0], "label", result, trainingRows))
        throw std::runtime_error("XGDMatrixSetFloatInfo failed.");


      //////////////////////////
      // make a boosted model
      BoosterHandle booster;
      initBooster(train, 1, &booster);

      // setup validation dataset
      DMatrixHandle h_validate;
      XGDMatrixCreateFromMat((float *) test, validateRows, ncol, MISSING, &h_validate);

      //////////////////////
      // learn
      double bestErr = std::numeric_limits<double>::max();
      bst_ulong bestLen;
      char *bestModel = NULL;
      unsigned bestIter;
      for (unsigned i = 0; i < MaxLearnIters_; i++) {
        int rv = XGBoosterUpdateOneIter(booster, i, train[0]);
        assert(rv == 0 && "learn step failed");

        /////////////
        // evaluate this new model
        bst_ulong out_len;
        const float *predict;
        rv = XGBoosterPredict(booster, h_validate, 0, 0, &out_len, &predict);

        assert(rv == 0 && "predict failed");
        assert(out_len == validateRows && "doesn't make sense!");

        // calculate Root Mean Squared Error (RMSE) of predicting our held-out set
        double err = 0.0;
        for (size_t i = 0; i < validateRows; i++) {
          double actual = testResult[i];
          double guess = predict[i];
#ifndef NDEBUG
          std::cout << "actual = " << actual
                    << ", guess = " << guess << "\n";
#endif
          err += std::pow(actual - guess, 2) / validateRows;
        }
        err = std::sqrt(err);

#ifndef NDEBUG
        std::cout << "RMSE = " << err << "\n\n";
#endif

        // is this model better than we've seen before?
        if (err <= bestErr || bestModel == NULL) {
          const char* ro_view;
          bst_ulong ro_len;
          rv = XGBoosterGetModelRaw(booster, &ro_len, &ro_view);
          assert(rv == 0 && "can't view model");

          // save this new best model
          bestErr = err;
          bestLen = ro_len;
          bestIter = i;

          std::free(bestModel); // drop the old model
          bestModel = (char*) std::malloc(bestLen);
          assert(bestModel != NULL);
          std::memcpy(bestModel, ro_view, ro_len);

        } else if (i - bestIter < MaxLearnPastBest_) {
          // we're willing to go beyond the minima so-far.
#ifndef NDEBUG
          std::cout << "going beyond best-seen\n";
#endif
        } else {
          // stop training, since it's not any getting better
          break;
        }
      }

      ////////
      // load the best model
      assert(bestModel != NULL);

#ifndef NDEBUG
        std::cout << "Using model with error: " << bestErr << "\n";
#endif

      BoosterHandle bestBooster;
      XGBoosterCreate(NULL, 0, &bestBooster);
      int rv =
        XGBoosterLoadModelFromBuffer(bestBooster, bestModel, bestLen);
      assert(rv == 0);

      // free our copy of the serialized model,
      // and drop the worse model
      std::free(bestModel);
      XGBoosterFree(booster);



      ////////
      // Next, we use the model to predict the running time of configurations
      // in the pool.
      ///////

      // build new configurations, aka test cases
      std::vector<KnobConfig> Test;
      float* testMat = (float*) std::malloc(SurrogateTestSz_ * ncol * sizeof(float));
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
      XGBoosterPredict(bestBooster, h_test, 0, 0, &out_len, &out);


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
#ifndef NDEBUG
        std::cout << "prediction[" << i << "]=" << out[i] << std::endl;
#endif
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
#ifndef NDEBUG
        std::cout << "chose config " << Chosen
                  << " with estimated time " << Entry.second << std::endl;
#endif
        Predictions_.push_back(Test[Chosen]);
      }

      // DONE

      // free memory related to the XGB objects.
      XGDMatrixFree(train[0]);
      XGBoosterFree(bestBooster);
      XGDMatrixFree(h_test);
      XGDMatrixFree(h_validate);
      free(testMat);

      return (Predictions_.size() > 0);
    }


    GenResult& saveConfig(KnobConfig KC) {
      auto Conf = std::make_shared<KnobConfig>(KC);
      auto FB = createFeedback(Cxt_->getFeedbackKind(), PREFERRED_FEEDBACK);

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
      std::free(test);
      std::free(testResult);
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

    // we often can produce another config, so we
    // need to bound the number of yes's given to avoid runaway
    // AOT compilation. Otherwise, we must say "no" when all predictions
    // are used up so that a training phase can occur.
    //
    // TODO(kavon): I'm not a fan of how the JIT pipeline has to be flushed
    // to retrain the surrogate model. It should occur in the pipeline.
    bool shouldCompileNext () override {
      const uint32_t BOUND = std::min(BatchSz_, (uint32_t) DEFAULT_COMPILE_AHEAD);
      bool aotLimiter = aheadOfTimeCount < BOUND;
      if (!aotLimiter)
        aheadOfTimeCount = 0;
      else
        aheadOfTimeCount++;

      return aotLimiter && Predictions_.size() > 0;
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
