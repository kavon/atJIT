#ifndef TUNER_FEEDBACK
#define TUNER_FEEDBACK

#include <cinttypes>
#include <chrono>
#include <optional>
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
};



class NoOpFeedback : public Feedback {
  using Token = uint64_t;

public:
  Token startMeasurement() override { return 0; }
  void endMeasurement(Token t) override { }
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
}; // end class



// TODO: make this resistant to recursion by using the tokens properly
// i.e., with a stack.
class ExecutionTime : public Feedback {
  using Token = uint64_t;

  std::chrono::time_point<std::chrono::system_clock> Start_;
  std::optional<int64_t> movingAvg{}; // cumulative
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

    if (movingAvg) {
      // https://en.wikipedia.org/wiki/Moving_average#Cumulative_moving_average
      movingAvg = (elapsedTime + ((dataPoints-1) * movingAvg.value())) / dataPoints;
    } else {
      // first measurement
      movingAvg = elapsedTime;
    }

    std::cout << "## avg = " << movingAvg.value() << " ns ##\n";
  }
}; // end class

} // end namespace


#endif // TUNER_FEEDBACK
