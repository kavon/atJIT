#pragma once

#include <cinttypes>
#include <climits>
#include <string>
#include <cassert>
#include <atomic>

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
  extern std::atomic<KnobID> KnobTicker;

  // Base class for tunable compiler "knobs", which
  // are simply tunable components.
  template< typename ValTy >
  class Knob {

  private:
    KnobID id__;

  public:
    Knob() {
      id__ = KnobTicker.fetch_add(1);
      assert(id__ != 0 && "exhausted the knob ticker!");
    }
    virtual ~Knob() = default;
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


  // represents a knob that can take on values in the range
  // [a, b], where a, b are scalar values.
  template < typename ValTy >
  class ScalarRange : public Knob<ValTy> {
  public:
    virtual ~ScalarRange() = default;

    // inclusive ranges
    virtual ValTy min() const = 0;
    virtual ValTy max() const = 0;

    template <typename S>
    friend bool operator== (ScalarRange<S> const&, ScalarRange<S> const&);

  }; // end class ScalarRange


  // a boolean-like scalar range
  class FlagKnob : public ScalarRange<int> {
  private:
    static constexpr int TRUE = 1;
    static constexpr int FALSE = 0;
    int current;
    int dflt;
  public:
    virtual ~FlagKnob() = default;
    FlagKnob(bool dflt_) : dflt(dflt_ ? TRUE : FALSE) {
      current = dflt;
    }
    int getDefault() const override { return dflt; }
    int getVal() const override { return current; }
    void setVal(int newVal) override {
      assert(newVal == TRUE || newVal == FALSE);
      current = newVal;
    }
    void apply(llvm::Module &M) override { } // do nothing by default
    int min() const override { return FALSE; }
    int max() const override { return TRUE; }

    bool getFlag() const {
      return current != FALSE;
    }

  }; // end class FlagKnob

////////////////////////
// handy type aliases and type utilities

namespace knob_type {
  using ScalarInt = tuner::ScalarRange<int>;
}

// this needs to appear first, before specializations.
template< typename Any >
struct is_knob {
  static constexpr bool value = false;
  using rawTy = void;
};


} // namespace tuner
