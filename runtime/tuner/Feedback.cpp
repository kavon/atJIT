#include <tuner/Feedback.h>
#include <loguru.hpp>

#include <chrono>
#include <cmath>

namespace tuner {

template <typename Duration = std::chrono::nanoseconds>
int64_t elapsedTime(Feedback::TimePoint Start, Feedback::TimePoint End) {
  Duration elapsedDur = (End - Start);
  return elapsedDur.count();
}

// NOTE: This implementation assumes that the sample
// data in each Feedback object is normally distributed.
bool Feedback::betterThan(Feedback& Other) {
  this->updateStats();
  Other.updateStats();

  /*
    We perform a two-sample t test aka Welch's unequal variances t-test.
    More details:
      1. https://en.wikipedia.org/wiki/Welch%27s_t-test
      2. Section 10.2 of Devore & Berk's
          "Modern Mathematical Statistics with Applications"

    Null Hypothesis:
      o_mean - my_mean <= delta

    Alternative Hypothesis 1:
      o_mean - my_mean > delta
            i.e. my mean is at least 'delta' lower than theirs, for
                 delta >= 0.

    Reject null hypothesis with confidence level 100(1-alpha)% if:
      test_statistic >= t_{alpha, degrees of freedom}

  */

  double my_mean = this->expectedValue();
  double my_var = this->variance();
  size_t my_sz = this->sampleSize();
  double my_scaledVar = my_var / my_sz;

  double o_mean = Other.expectedValue();
  double o_var = Other.variance();
  size_t o_sz = Other.sampleSize();
  double o_scaledVar = o_var / o_sz;

  const double DELTA = 0.02 * o_mean; // delta is defined as a % of their mean.

  double test_statistic = (o_mean - my_mean - DELTA) /
                          std::sqrt(o_scaledVar + my_scaledVar);

  // degrees of freedom
  double df = std::trunc( // round down
                std::pow(o_scaledVar + my_scaledVar, 2) /
                    ( (std::pow(o_scaledVar, 2) / (o_sz - 1))
                    + (std::pow(my_scaledVar, 2) / (my_sz - 1))
                  ));

  DLOG_F(INFO, "delta = %.3f, ts = %.3f, df = %.1f", DELTA, test_statistic, df);

  // TODO: determine the _correct_ critical values
  // for a given confidence level to determine
  // whether to reject null hyp.

  // hardcode alpha = 0.05, df = 6
  const double THRESH = 1.943;

  return test_statistic >= THRESH;
}

void calculateBasicStatistics(
                                std::vector<Feedback::TimePoint>& startBuf,
                                std::vector<Feedback::TimePoint>& endBuf,
                                size_t sampleSz,
                                double& sampleAvg,
                                double& sampleVariance,
                                double& sampleErr
                                ) {

  CHECK_F(sampleSz > 0, "calculating statistics when there is no data.");

  { // compute sample average
    int64_t totalTime = 0;
    for (size_t i = 0; i < sampleSz; i++) {
      int64_t obsTime = elapsedTime(startBuf[i], endBuf[i]);

      DCHECK_F(obsTime > 0, "saw bogus sample time!");
      DCHECK_F(std::numeric_limits<int64_t>::max() - totalTime > obsTime,
                        "overflow. use a different time unit!");

      totalTime += obsTime;
    }
    sampleAvg = ((double) totalTime) / sampleSz;
  }

  { // compute sample variance and standard error of the mean
    if (sampleSz == 1) {
      sampleVariance = 0;
      sampleErr = 0;
    } else {
      int64_t sumSqDiff = 0;
      for (size_t i = 0; i < sampleSz; i++) {
        sumSqDiff += std::pow(elapsedTime(startBuf[i], endBuf[i]) - sampleAvg, 2);
      }
      sampleVariance = ((double) sumSqDiff) / (sampleSz - 1);
      sampleErr = std::sqrt(sampleVariance) / std::sqrt(sampleSz);
    }
  }
}


std::shared_ptr<Feedback> createFeedback(FeedbackKind requested,
                                        std::optional<FeedbackKind> preferred) {
  switch (requested) {
    case FB_None:
      if (preferred)
        return createFeedback(preferred.value(), std::nullopt);

      return std::make_shared<NoOpFeedback>();

    case FB_Total:
      return std::make_shared<TotalExecutionTime>();

    case FB_Total_IgnoreError:
      return std::make_shared<TotalExecutionTime>(-1);

    case FB_Recent:
      return std::make_shared<RecentExecutionTime>();

    case FB_Recent_NP:
    default:
      throw std::runtime_error("createFeedback -- unknown feedback kind!");
  };
}



} // end namespace
