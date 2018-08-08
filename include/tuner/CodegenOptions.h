#include <tuner/Knob.h>

namespace tuner {

class CodeGenOptLvl : public ScalarRange<int> {
private:
  int current;
  int dflt;
public:
  CodeGenOptLvl(int dflt_ = 3) : dflt(dflt_), current(dflt_) {}
  int getDefault() const override { return dflt; }
  int getVal() const override { return current; }
  void setVal(int newVal) override { current = newVal; }
  void apply(llvm::Module &M) override { } // can't set it in the module.
  int min() const override { return 0; }
  int max() const override { return 3; }
  std::string getName() const override {
     return "codegen opt level";
  }

  llvm::CodeGenOpt::Level getLevel() {
    switch (current) {
      case 0:
        return llvm::CodeGenOpt::Level::None;
      case 1:
        return llvm::CodeGenOpt::Level::Less;
      case 2:
        return llvm::CodeGenOpt::Level::Default;
      case 3:
        return llvm::CodeGenOpt::Level::Aggressive;
      default:
        throw std::logic_error("invalid codegen optimization level.");
    };
  }
}; // end class

class FastISelOption : public FlagKnob {
public:
  FastISelOption() : FlagKnob(false) {}
  std::string getName() const override {
     return "use FastISel";
  }
}; // end class

} // end namespace
