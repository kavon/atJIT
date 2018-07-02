#ifndef TUNER_TUNER
#define TUNER_TUNER

#include <tuner/Feedback.h>

namespace tuner {

  class Tuner {

  public:
    // ensures derived destructors are called
    virtual ~Tuner() = default;

    virtual void applyConfiguration (Feedback &prior) = 0;

  }; // end class Tuner

  class NoOpTuner : public Tuner {

    void applyConfiguration (Feedback &prior) override {}
  };

} // namespace tuner

#endif // TUNER_TUNER
