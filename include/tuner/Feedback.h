#ifndef TUNER_FEEDBACK
#define TUNER_FEEDBACK

#include <cinttypes>
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
};

} // end namespace


#endif // TUNER_FEEDBACK
