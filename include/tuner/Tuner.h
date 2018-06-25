#ifndef TUNER_TUNER
#define TUNER_TUNER

#include <vector>

#include <tuner/Knob.h>
#include <tuner/Feedback.h>

namespace tuner {

  // Base class for knob tuning
  class Tuner {

  public:
    virtual void applyConfiguration(Feedback &prior) = 0;

  }; // end class Tuner

} // namespace tuner

#endif // TUNER_TUNER
