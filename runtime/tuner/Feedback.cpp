#include <tuner/Feedback.h>

namespace tuner {


void calculate_parametric_statistics(
                                std::vector<Feedback::TimePoint>& startBuf,
                                std::vector<Feedback::TimePoint>& endBuf,
                                size_t sampleSz,
                                double& average,
                                double& sampleVariance) {

  throw std::runtime_error("implement calculate_parametric_statistics");
}


std::shared_ptr<Feedback> createFeedback(FeedbackKind requested, std::optional<FeedbackKind> preferred) {
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
