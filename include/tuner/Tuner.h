#ifndef TUNER_TUNER
#define TUNER_TUNER

#include <tuner/Feedback.h>

namespace tuner {

  class Tuner {

  public:
    // ensures derived destructors are called
    virtual ~Tuner() = default;

    virtual void applyConfiguration (std::shared_ptr<Feedback> current) = 0;

  }; // end class Tuner

  class NoOpTuner : public Tuner {

    void applyConfiguration (std::shared_ptr<Feedback> current) override {}
  };

} // namespace tuner

#endif // TUNER_TUNER
