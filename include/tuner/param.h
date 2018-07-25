#ifndef TUNER_PARAM
#define TUNER_PARAM

#include <tuner/Knob.h>
#include <cassert>

namespace tuned_param {

class IntRange : private tuner::ScalarRange<int> {
private:
  int lo_, hi_;
  int cur, dflt_;

  int min() const override { return lo_; }
  int max() const override { return hi_; }
  int getVal() const override { return cur; }
  int getDefault() const override { return dflt_; }
  void apply(llvm::Module &M) override {}
  void setVal(int val) override {
    assert(lo_ <= val && val <= hi_);
    cur = val;
  }

public:

  IntRange(int lo, int hi) : IntRange(lo, hi, lo) {}

  IntRange(int lo, int hi, int dflt) : lo_(lo), hi_(hi),
                                       dflt_(dflt) {
    assert(lo_ <= hi_ && "range doesn't make sense");
    assert(lo_ <= dflt && dflt <= hi_ && "default doesn't make sense");
    setVal(dflt);
  }
};




}

#endif // TUNER_PARAM
