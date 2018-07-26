#ifndef TUNER_PARAM
#define TUNER_PARAM

#include <tuner/Knob.h>

#include <cassert>
#include <cinttypes>

namespace tuned_param {

class IntRange : public tuner::ScalarRange<int> {
private:
  int lo_, hi_;
  int cur, dflt_;

public:

  int min() const override { return lo_; }
  int max() const override { return hi_; }
  int getVal() const override { return cur; }
  int getDefault() const override { return dflt_; }
  void apply(llvm::Module &M) override {}
  void setVal(int val) override {
    assert(lo_ <= val && val <= hi_);
    cur = val;
  }

  std::string getName() const override {
     return "tunable param id " + std::to_string(getID());
  }

  IntRange(int lo, int hi) : IntRange(lo, hi, lo) {}

  IntRange(int lo, int hi, int dflt) : lo_(lo), hi_(hi),
                                       dflt_(dflt) {
    assert(lo_ <= hi_ && "range doesn't make sense");
    assert(lo_ <= dflt && dflt <= hi_ && "default doesn't make sense");
    setVal(dflt);
  }

  size_t hash() const {
    return max() ^ min() ^ getDefault();
  }

  bool operator==(IntRange const&);
};

} // end namespace tuned_param

namespace tuner {
  template< >
  struct is_knob< tuned_param::IntRange > {
    static constexpr bool value = true;
    using rawTy = int;
  };
}

namespace std {
  template<> struct hash<tuned_param::IntRange>
  {
    typedef tuned_param::IntRange argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
      return s.hash();
    }
  };
} // end namespace std

#endif // TUNER_PARAM
