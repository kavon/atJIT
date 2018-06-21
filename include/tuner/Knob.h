#ifndef TUNER_KNOBS
#define TUNER_KNOBS

namespace tuner {

  // Base class for tunable compiler "knobs", which
  // are simply tunable components.
  template< class TunedValTy >
  class Knob {

  public:
    virtual TunedValTy getVal() const = 0;
    virtual void setVal(TunedValTy) = 0;

  }; // end class Knob

} // namespace tuner

#endif // TUNER_KNOBS
