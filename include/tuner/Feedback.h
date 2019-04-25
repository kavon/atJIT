#pragma once

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

#include <easy/runtime/Context.h>

/////
// collects data and other information about the quality of configurations.
// for subsequent feedback-directed reoptimization.

namespace tuner {

/////////////// BASE CLASS ///////////////

class Feedback {
public:
  using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

  virtual ~Feedback() = default;
  virtual TimePoint startMeasurement() = 0;
  virtual void endMeasurement(TimePoint) = 0;

  virtual bool betterThan(Feedback& Other) {
    // lower times are better, where `this` is selfish
    // and says its better with an equivalent measure.

    double mine = this->avgMeasurement();
    double theirs = Other.avgMeasurement();

    return mine <= theirs;
  }

  virtual bool goodQuality() { return false; }

  // Consult goodQuality to determine if this value is useful.
  virtual double avgMeasurement() {
    return std::numeric_limits<double>::max();
  }

  virtual void dump(std::ostream &os) = 0;

  // default handlers for the event that the underlying object we're collecting
  // feedback for has seen a "new deployment" underwhich feedback
  // will be recorded, and accessing that value.
  virtual void resetDeployedTime() {};
  virtual uint64_t getDeployedTime() { return ~0ULL; }
};



/////////////// UTILITY FUNCTIONS ///////////////

void calculate_parametric_statistics(
                                std::vector<Feedback::TimePoint>& startBuf,
                                std::vector<Feedback::TimePoint>& endBuf,
                                size_t sampleSz,
                                double& average,
                                double& sampleVariance
                              );



/////////////// DERIVED CLASSES ///////////////

class NoOpFeedback : public Feedback {
private:
  TimePoint dummy;
public:
  NoOpFeedback() : dummy(std::chrono::steady_clock::now()) { }
  TimePoint startMeasurement() override { return dummy; }
  void endMeasurement(TimePoint t) override { }

  virtual void dump(std::ostream &os) override {}
}; // end class



// does not compute an average, etc. nice for debugging.
class DebuggingFB : public Feedback {
  TimePoint startMeasurement() override {
    return std::chrono::steady_clock::now();
  }

  void endMeasurement(TimePoint Start) override {
    auto End = std::chrono::steady_clock::now();
    std::chrono::duration<int64_t, std::nano> elapsed = (End - Start);
    std::cout << "== elapsed time: " << elapsed.count() << " ns ==\n";
  }

  void dump(std::ostream &os) override { }

}; // end class


class RecentFeedbackBuffer : public Feedback {
private:
  // circular buffer of samples.
  // cur is the index of the oldest sample.
  size_t cur = 0;

protected:
  std::atomic<size_t> observations = 0; // total. only most recent are tracked in detail.
  std::atomic<uint64_t> deployedTime = 0; // total.

  std::mutex bufLock;
  std::vector<TimePoint> startBuf;
  std::vector<TimePoint> endBuf;
  size_t bufSz;

public:
  RecentFeedbackBuffer(size_t n = 10) : bufSz(n), startBuf(n), endBuf(n) { }

  ~RecentFeedbackBuffer() { }

  TimePoint startMeasurement() override {
    return std::chrono::steady_clock::now();
  }

  void endMeasurement(TimePoint Start) override {
    auto End = std::chrono::steady_clock::now();

    /////// update the circular buffer

    // TODO: make thread-specific buffers to avoid the need for a lock.

    //////////////////////////////////////
    // START the critical section
    bufLock.lock();
    // TODO: I don't really think we need to be tracking total deployed time.
    // we can cut overhead out by not tracking it.
    std::chrono::duration<int64_t, std::nano> elapsedDur = (End - Start);
    int64_t elapsedTime = elapsedDur.count();
    assert(elapsedTime > 0 && "encountered a negative time?");

    startBuf[cur] = Start;
    endBuf[cur] = End;
    cur = (cur + 1) % bufSz;
    observations++;
    deployedTime += elapsedTime;

    //////////////////////////////////////
    // END of critical section
    bufLock.unlock();
  }

  void resetDeployedTime() override {
    deployedTime = 0;
  }

  uint64_t getDeployedTime() override {
    return deployedTime;
  }

  virtual void dump(std::ostream &os) = 0;

};



class RecentExecutionTime : public RecentFeedbackBuffer {
private:
  size_t lastCalc = 0;

  size_t sampleSize;
  double average;
  double sampleVariance;

  // TODO: this might need to become a public member of the base class.
  void updateStats() {
    // new observations have come in since this method
    // was last called, so we recalulate things.

    bufLock.lock();
    lastCalc = observations;
    sampleSize = std::min(bufSz, lastCalc);

    std::vector<TimePoint> startBufCopy(startBuf);
    std::vector<TimePoint> endBufCopy(endBuf);
    bufLock.unlock();

    // perform calculations without lock held
    calculate_parametric_statistics(startBufCopy, endBufCopy, sampleSize, average, sampleVariance);
  }

public:

  RecentExecutionTime() : RecentFeedbackBuffer(64) {}

  virtual bool goodQuality() override { return false; }

  virtual double avgMeasurement() override {
    if (lastCalc == observations)
      return average;

    updateStats();
    return average;
  }

  virtual bool betterThan(Feedback& Other) override { return false; }

  virtual void dump(std::ostream &os) {
    JSON::beginObject(os);

    JSON::output(os, "feedback_kind", "recent_execution");

    JSON::endObject(os);
  }

};



class TotalExecutionTime : public RecentFeedbackBuffer {
private:
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
  TotalExecutionTime(double errPctBound = DEFAULT_STD_ERR_PCT)
      : errBound(errPctBound) {}


  TimePoint startMeasurement() override {
    return std::chrono::steady_clock::now();
  }

  void endMeasurement(TimePoint Start) override {
    auto End = std::chrono::steady_clock::now();
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

  double avgMeasurement() override {
    ////////////////////////////////////////////////////////////////////
    // START the critical section
    protecc.lock();

    double retVal = std::numeric_limits<double>::max();
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


// create a feedback object based on the user's request.
// if the user requested "None", then the caller's preference is used
// instead (which may also be None).
std::shared_ptr<Feedback> createFeedback(FeedbackKind requested, std::optional<FeedbackKind> preferred);



} // end namespace
