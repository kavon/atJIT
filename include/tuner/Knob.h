#ifndef TUNER_KNOBS
#define TUNER_KNOBS

#include <cinttypes>
#include <climits>

namespace tuner {

  // polymorphism of Knobs is primarily achieved through inheritance.

  // Base class for tunable compiler "knobs", which
  // are simply tunable components.
  template< typename ValTy >
  class Knob {

  public:
    // value accessors
    virtual ValTy getVal() const = 0;
    virtual void setVal(ValTy) = 0;

  }; // end class Knob


  template < typename ValTy >
  class ScalarKnob : public Knob<ValTy> {

  public:
    // inclusive ranges
    virtual ValTy min() const = 0;
    virtual ValTy max() const = 0;
  }; // end class ScalarKnob

} // namespace tuner

#endif // TUNER_KNOBS
