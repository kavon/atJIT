#ifndef TUNER_KNOBS
#define TUNER_KNOBS

#include <cinttypes>
#include <climits>
#include <string>

#include <tuner/Util.h>

#include <llvm/IR/Module.h>

namespace tuner {

  // polymorphism of Knobs is primarily achieved through inheritance.

  // I really wish c++ supported template methods that are virtual!
  // We implement the template portion manually with macros.
  // If you see things like "HANDLE_CASE" that take a type as a parameter,
  // that's what we mean. This is still robust, since a new virtual method will
  // cause the compiler to point out places where you haven't updated your code

  using KnobID = uint64_t;

  // used to ensure knob IDs are unique.
  // we rely on the fact that 0 is an invalid knob ID
  extern KnobID KnobTicker;

  // Base class for tunable compiler "knobs", which
  // are simply tunable components.
  template< typename ValTy >
  class Knob {

  private:
    KnobID id__;

  public:
    Knob() {
      id__ = KnobTicker++;
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
    KnobID getID() const { return id__; }

    virtual std::string getName() const {
       return "knob id " + std::to_string(getID());
    }

    // members related to exporting to a flat array

    virtual size_t size() const { return 1; } // num values to be flattened

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
