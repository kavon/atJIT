#ifndef TUNER_FEEDBACK
#define TUNER_FEEDBACK

#include <cinttypes>
#include <chrono>
#include <cmath>
#include <cassert>
#include <iostream>

/////
// collects data and other information about the quality of configurations.
// for subsequent feedback-directed reoptimization.

namespace tuner {

class Feedback {
  using Token = uint64_t;

public:
  virtual ~Feedback() = default;
  virtual Token startMeasurement() = 0;
  virtual void endMeasurement(Token) = 0;

  virtual double avgMeasurement() = 0;

  virtual void dump() const = 0;
};



class NoOpFeedback : public Feedback {
  using Token = uint64_t;

public:
  Token startMeasurement() override { return 0; }
  void endMeasurement(Token t) override { }
  double avgMeasurement() override { return 0; }

  virtual void dump() const override {
    std::cout << "NoOpFeedback did not measure anything.\n";
  }
}; // end class



// does not compute an average, etc. nice for debugging.
class DebuggingFB : public Feedback {
  using Token = uint64_t;

  std::chrono::time_point<std::chrono::system_clock> Start_;

  Token startMeasurement() override {
    Start_ = std::chrono::system_clock::now();
    return 0;
  }

  void endMeasurement(Token t) override {
    auto End = std::chrono::system_clock::now();
    std::chrono::duration<int64_t, std::nano> elapsed = (End - Start_);
    std::cout << "== elapsed time: " << elapsed.count() << " ns ==\n";
  }

  double avgMeasurement() override { return 0; }

  void dump() const override {
    std::cout << "DebuggingFB does not save measurements\n";
  }

}; // end class



// TODO: make this resistant to recursion by using the tokens properly
// i.e., with a stack.
class ExecutionTime : public Feedback {
  using Token = uint64_t;

  std::chrono::time_point<std::chrono::system_clock> Start_;


  double average = 0; // cumulative
  double sampleVariance = 0; // unbiased sample variance
  double stdDev = 0; // unbiased sample standard deviation
  double stdError = 0;
  double stdErrorPct = 0;
  double sumSqDiff = 0;
  uint64_t dataPoints = 0;


  Token startMeasurement() override {
    Start_ = std::chrono::system_clock::now();
    return dataPoints;
  }

  void endMeasurement(Token t) override {
    assert(t == dataPoints && "out of order datapoints"); // probably recursion
    dataPoints++;

    auto End = std::chrono::system_clock::now();
    std::chrono::duration<int64_t, std::nano> elapsedDur = (End - Start_);
    int64_t elapsedTime = elapsedDur.count();

    if (dataPoints == 1) {
      average = elapsedTime;
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

  }

  double avgMeasurement() override {
    return average;
  }

  void dump () const override {

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
  }
}; // end class

} // end namespace


#endif // TUNER_FEEDBACK
