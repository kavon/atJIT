#ifndef TUNER_KNOBS
#define TUNER_KNOBS

#include <cinttypes>
#include <climits>
#include <string>

#include <llvm/IR/Module.h>

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
    virtual ValTy getDefault() const = 0;
    virtual ValTy getVal() const = 0;
    virtual void setVal(ValTy) = 0;

    virtual void apply(llvm::Module &M) = 0;

    // a unique ID relative to all knobs in the process.
    // since multiple instances of autotuners can be created
    // per process, this only guarentees uniqueness of each
    // instance, it is otherwise unstable.
    KnobID getID() const { return id; }

    virtual std::string getName() const {
       return "knob id " + std::to_string(getID());
    }

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
