#ifndef TUNER_FEEDBACK
#define TUNER_FEEDBACK

#include <optional>
#include <cinttypes>
#include <chrono>
#include <cmath>
#include <cassert>
#include <iostream>
#include <mutex>
#include <cfloat>

#include <tuner/Util.h>

/////
// collects data and other information about the quality of configurations.
// for subsequent feedback-directed reoptimization.

namespace tuner {

class Feedback {
public:
  using Token = std::chrono::time_point<std::chrono::system_clock>;

  virtual ~Feedback() = default;
  virtual Token startMeasurement() = 0;
  virtual void endMeasurement(Token) = 0;

  bool betterThan(Feedback& Other) {
    // lower times are better, where `this` is selfish
    // and says its better with an equivalent measure.

    std::optional<double> mine = this->avgMeasurement();
    std::optional<double> theirs = Other.avgMeasurement();

    return mine.value_or(DBL_MAX) <= theirs.value_or(DBL_MAX);
  }

  // "None" indicates that there is no accurate average available yet.
  virtual std::optional<double> avgMeasurement() {
    return std::nullopt;
  }

  virtual void dump() = 0;
};



class NoOpFeedback : public Feedback {
private:
  Token dummy;
public:
  NoOpFeedback() : dummy(std::chrono::system_clock::now()) { }
  Token startMeasurement() override { return dummy; }
  void endMeasurement(Token t) override { }

  virtual void dump() override {
    std::cout << "NoOpFeedback did not measure anything.\n";
  }
}; // end class



// does not compute an average, etc. nice for debugging.
class DebuggingFB : public Feedback {
  Token startMeasurement() override {
    return std::chrono::system_clock::now();
  }

  void endMeasurement(Token Start) override {
    auto End = std::chrono::system_clock::now();
    std::chrono::duration<int64_t, std::nano> elapsed = (End - Start);
    std::cout << "== elapsed time: " << elapsed.count() << " ns ==\n";
  }

  void dump() override {
    std::cout << "DebuggingFB does not save measurements\n";
  }

}; // end class


class ExecutionTime : public Feedback {

  //////////////
  // this lock protects all fields of this object.
  std::mutex protecc;

  double average = 0; // cumulative
  double sampleVariance = 0; // unbiased sample variance
  double stdDev = 0; // unbiased sample standard deviation
  double stdError = 0;
  double stdErrorPct = 0;
  double sumSqDiff = 0;
  double errBound = 0; // a precentage
  uint64_t dataPoints = 0;

public:
  //////////////////////////
  // a value < 0 says:   ignore the stdErr and always return the average, if
  //                     it exists
  // a value >= 0 says:  return if you have at least 2 observations, where
  //                     the precent std err of the mean is <= the value.
  ExecutionTime(double errPctBound = DEFAULT_STD_ERR_PCT)
      : errBound(errPctBound) {}

  Token startMeasurement() override {
    return std::chrono::system_clock::now();
  }

  void endMeasurement(Token Start) override {
    auto End = std::chrono::system_clock::now();
    std::chrono::duration<int64_t, std::nano> elapsedDur = (End - Start);

    int64_t elapsedTime = elapsedDur.count();

    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    dataPoints++;

    if (dataPoints == 1) {
      average = elapsedTime;

      ////////////////////////////////////////////////////////////////////
      // END of critical section
      protecc.unlock();
      return;
    }

    /////////////
    // sources for these forumlas:
    //
    // https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
    // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
    // https://en.wikipedia.org/wiki/Standard_deviation#Unbiased_sample_standard_deviation
    // https://en.wikipedia.org/wiki/Standard_error#Estimate

    const double N = dataPoints;

    const double newAvg = (elapsedTime + ((N-1) * average)) / N;

    sumSqDiff += (elapsedTime - average) * (elapsedTime - newAvg);
    sampleVariance = sumSqDiff / (N-1);

    // we use the approximation of the correction for a normal distribution.
    stdDev = std::sqrt(sumSqDiff / (N - 1.5));

    stdError = stdDev / std::sqrt(N);

    stdErrorPct = (stdError / newAvg) * 100.0;

    average = newAvg;

    ////////////////////////////////////////////////////////////////////
    // END of critical section
    protecc.unlock();
    return;
  }

  std::optional<double> avgMeasurement() override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    std::optional<double> retVal = std::nullopt;
    if (   (dataPoints > DEFAULT_MIN_TRIALS && stdErrorPct <= errBound)
        || ((dataPoints >= 1) && errBound < 0) ) {
      retVal = average;
    }
    ////////////////////////////////////////////////////////////////////
    // END of critical section
    protecc.unlock();
    return retVal;
  }

  void dump () override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    std::cout << "avg time: ";
    if (dataPoints)
      std::cout << average << " ns";
    else
      std::cout << "< none >";
    std::cout << ", ";

    std::cout << "error: ";
    if (dataPoints)
      std::cout << stdErrorPct << "%";
    else
      std::cout << "< none >";
    std::cout << ", ";


    // Other less interesting stats in a compressed output
    std::cout << "(s^2: ";
    if (dataPoints)
      std::cout << sampleVariance;
    else
      std::cout << "X";
    std::cout << ", ";

    std::cout << "s: ";
    if (dataPoints)
      std::cout << stdDev;
    else
      std::cout << "X";
    std::cout << ", ";

    std::cout << "SEM: ";
    if (dataPoints)
      std::cout << stdError;
    else
      std::cout << "X";

    std::cout << ") ";

    std::cout << "from " << dataPoints << " measurements\n";

    ////////////////////////////////////////////////////////////////////
    // END of critical section
    protecc.unlock();
  }
}; // end class

} // end namespace


#endif // TUNER_FEEDBACK
