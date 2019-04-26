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

void calculateParametricStatistics(
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
