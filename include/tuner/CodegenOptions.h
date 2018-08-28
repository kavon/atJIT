#include <tuner/Knob.h>

#include <llvm/Analysis/InlineCost.h>

namespace tuner {

// a typical, simple integer range knob
class SimpleRange : public ScalarRange<int> {
private:
  int dflt;
  int current;
public:
  SimpleRange(int dflt_) : dflt(dflt_), current(dflt_) {}
  int getDefault() const override { return dflt; }
  int getVal() const override { return current; }
  void setVal(int newVal) override { current = newVal; }
  void apply(llvm::Module &M) override { } // can't set it in the module.
}; // end class

class FastISelOption : public FlagKnob {
public:
  FastISelOption(bool dflt_ = false) : FlagKnob(dflt_) {}
  std::string getName() const override {
     return "use FastISel";
  }
}; // end class

class IPRAOption : public FlagKnob {
public:
  IPRAOption(bool dflt_ = false) : FlagKnob(dflt_) {}
  std::string getName() const override {
     return "use IPRA";
  }
}; // end class

  // NOTE: we turned off the O0 option for now since
  // it is an awful code generator.
class CodeGenOptLvl : public SimpleRange {
public:
  CodeGenOptLvl(int dflt_ = 3) : SimpleRange(dflt_) {}
  int min() const override { return 1; }
  int max() const override { return 3; }

  std::string getName() const override {
     return "codegen opt level";
  }

  llvm::CodeGenOpt::Level getLevel() {
    switch (getVal()) {
      case 1:
        return llvm::CodeGenOpt::Level::Less;
      case 2:
        return llvm::CodeGenOpt::Level::Default;
      case 3:
        return llvm::CodeGenOpt::Level::Aggressive;

      case 0:
          // return llvm::CodeGenOpt::Level::None;  // see NOTE above
      default:
        throw std::logic_error("invalid codegen optimization level.");
    };
  }
}; // end class


class OptimizerOptLvl : public SimpleRange {
public:
  OptimizerOptLvl(int dflt_ = 3) : SimpleRange(dflt_) {}
  int min() const override { return 0; }
  int max() const override { return 3; }

  std::string getName() const override {
     return "optimizer opt level";
  }
}; // end class

// NOTE: I believe it's:
//     0 -> no size optimization
//     1 -> -Os
//     2 -> -Oz
class OptimizerSizeLvl : public SimpleRange {
public:
  OptimizerSizeLvl(int dflt_ = 0) : SimpleRange(dflt_) {}
  int min() const override { return 0; }
  int max() const override { return 2; }

  std::string getName() const override {
     return "optimizer size opt level";
  }
}; // end class

class InlineThreshold : public ScalarRange<int> {
  llvm::InlineParams Params;
  llvm::InlineParams DefaultParams;

public:
  InlineThreshold() {
    Params = llvm::getInlineParams();
    DefaultParams = Params;
    assert(getVal() <= max() && getVal() >= min());
  }

  InlineThreshold(unsigned OptLevel, unsigned SizeOptLevel) {
    Params = llvm::getInlineParams(OptLevel, SizeOptLevel);
    DefaultParams = Params;
    assert(getVal() <= max() && getVal() >= min());
  }

  void setVal(int Threshold) override {
    Params.DefaultThreshold = Threshold;
  }

  int getVal() const override {
    return Params.DefaultThreshold;
  }

  int getDefault() const override {
    return DefaultParams.DefaultThreshold;
  }

  int min() const override {
    return -2000;
  }

  int max() const override {
    return 2000;
  }

  std::string getName() const override {
    return "inlining threshold";
  }

  void apply(llvm::Module &M) override { }
};

} // end namespace
