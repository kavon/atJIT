#ifndef TUNER_FEEDBACK
#define TUNER_FEEDBACK

#include <cinttypes>
#include <chrono>
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

} // end namespace


#endif // TUNER_FEEDBACK
