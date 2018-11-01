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
#include <tuner/JSON.h>

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

  virtual bool goodQuality() { return false; }

  // "None" indicates that there is no accurate average available yet.
  virtual std::optional<double> avgMeasurement() {
    return std::nullopt;
  }

  virtual void dump(std::ostream &os) = 0;

  // default handlers for the event that the underlying object we're collecting
  // feedback for has seen a "new deployment" underwhich feedback
  // will be recorded, and accessing that value.
  virtual void newDeployment() {};
  virtual uint64_t getDeployedTime() { return ~0ULL; }
};



class NoOpFeedback : public Feedback {
private:
  Token dummy;
public:
  NoOpFeedback() : dummy(std::chrono::system_clock::now()) { }
  Token startMeasurement() override { return dummy; }
  void endMeasurement(Token t) override { }

  virtual void dump(std::ostream &os) override {}
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

  void dump(std::ostream &os) override { }

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

  uint64_t deployedTime = 0; // total execution time keeper since the last newDeployment.

public:
  //////////////////////////
  // a value < 0 says:   ignore the stdErr and always return the average, if
  //                     it exists
  // a value >= 0 says:  return if you have at least 2 observations, where
  //                     the precent std err of the mean is <= the value.
  ExecutionTime(double errPctBound = DEFAULT_STD_ERR_PCT)
      : errBound(errPctBound) {}

  void newDeployment() override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    deployedTime = 0;

    ////////////////////////////////////////////////////////////////////
    // END of critical section
    protecc.unlock();
  }

  // TODO(kavon): is a critical section really needed here on the read?
  uint64_t getDeployedTime() override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    uint64_t retVal;

    protecc.lock();

    retVal = deployedTime;

    ////////////////////////////////////////////////////////////////////
    // END of critical section
    protecc.unlock();

    return retVal;
  }

  Token startMeasurement() override {
    return std::chrono::system_clock::now();
  }

  void endMeasurement(Token Start) override {
    auto End = std::chrono::system_clock::now();
    std::chrono::duration<int64_t, std::nano> elapsedDur = (End - Start);

    int64_t elapsedTime = elapsedDur.count();

    assert(elapsedTime > 0 && "encountered a negative time?");

    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    // calculate new deployed time with overflow check.
    uint64_t newDeployedTime = deployedTime + elapsedTime;
    if (newDeployedTime < deployedTime)
      newDeployedTime = ~0ULL;

    deployedTime = newDeployedTime;

    dataPoints++;

    if (dataPoints == 1) {
      average = elapsedTime;

      ////////////////////////////////////////////////////////////////////
      // END of critical section
      protecc.unlock();
      return;
    } // end if

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
  } // end of endMeasurement

  bool goodQuality() override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    bool ans = (dataPoints > DEFAULT_MIN_TRIALS && stdErrorPct <= errBound)
               || ((dataPoints >= 1) && errBound < 0) ;

   ////////////////////////////////////////////////////////////////////
   // END of critical section
   protecc.unlock();
   return ans;
  }

  std::optional<double> avgMeasurement() override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    std::optional<double> retVal = std::nullopt;
    if (dataPoints >= 1) {
      retVal = average;
    }
    ////////////////////////////////////////////////////////////////////
    // END of critical section
    protecc.unlock();
    return retVal;
  }

  void dump (std::ostream &os) override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    JSON::beginObject(os);

    JSON::output(os, "measurements", dataPoints, dataPoints != 0);

    if (dataPoints != 0) {
      JSON::output(os, "unit", "nano");
      JSON::output(os, "time", average);
      JSON::output(os, "std_error_pct", stdErrorPct);
      JSON::output(os, "variance", sampleVariance);
      JSON::output(os, "std_dev", stdDev);
      JSON::output(os, "std_error_mean", stdError, false);
    }

    JSON::endObject(os);

    ////////////////////////////////////////////////////////////////////
    // END of critical section
    protecc.unlock();
  }
}; // end class

} // end namespace


#endif // TUNER_FEEDBACK
