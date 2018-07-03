#ifndef TUNER_KNOBS
#define TUNER_KNOBS

#include <cinttypes>
#include <climits>

namespace tuner {

  // polymorphism of Knobs is primarily achieved through inheritance.

  using KnobID = uint64_t;

namespace {
  // used to ensure knob IDs are unique.
  static KnobID KnobTicker = 1;
}

  // Base class for tunable compiler "knobs", which
  // are simply tunable components.
  template< typename ValTy >
  class Knob {

  private:
    KnobID id;

  public:
    Knob() {
      id = KnobTicker++;
    }
    // value accessors
    virtual ValTy getVal() const = 0;
    virtual void setVal(ValTy) = 0;

    // a unique ID relative to all knobs in the program.
    KnobID getID() const { return id; }

  }; // end class Knob


  template < typename ValTy >
  class ScalarKnob : public Knob<ValTy> {

  public:
    // inclusive ranges
    virtual ValTy min() const = 0;
    virtual ValTy max() const = 0;
  }; // end class ScalarKnob


// handy type aliases.
namespace knob_type {
  using Int = tuner::ScalarKnob<int>;
}


} // namespace tuner

#endif // TUNER_KNOBS
