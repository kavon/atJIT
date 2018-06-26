#ifndef TUNER_TUNER
#define TUNER_TUNER

#include <vector>

#include <tuner/Knob.h>
#include <tuner/Feedback.h>

namespace tuner {

  class TunerBase {

  public:
    virtual void applyConfiguration(Feedback &prior) { };

  }; // end class TunerBase

} // namespace tuner

#endif // TUNER_TUNER
