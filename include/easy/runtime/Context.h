#ifndef CONTEXT
#define CONTEXT

#include <vector>
#include <memory>
#include <cstdint>
#include <cstring>

#include <easy/runtime/Function.h>
#include <tuner/param.h>
#include <tuner/Feedback.h>

namespace tuner {
  enum AutoTuner {
    AT_None = 0,
    AT_Random,
    AT_Bayes,
    AT_Anneal
  };

  static std::string TunerName(AutoTuner AT) {
    switch (AT) {
      case AT_None   : return "none";
      case AT_Random : return "random";
      case AT_Bayes  : return "bayes";
      case AT_Anneal : return "anneal";
      default: throw std::runtime_error("unknown tuner name");
    }
  }

  static const std::vector<AutoTuner> AllTuners = {AT_None, AT_Random, AT_Bayes, AT_Anneal};
}

namespace easy {

struct ArgumentBase {

  enum ArgumentKind {
    AK_Forward,
    AK_Int,
    AK_Float,
    AK_Ptr,
    AK_Struct,
    AK_Module,
    AK_IntRange,
  };

  ArgumentBase() = default;
  virtual ~ArgumentBase() = default;

  bool operator==(ArgumentBase const &Other) const {
    return this->kind() == Other.kind() &&
        this->compareWithSameType(Other);
  }

  template<class ArgTy>
  std::enable_if_t<std::is_base_of<ArgumentBase, ArgTy>::value, ArgTy const*>
  as() const {
    if(kind() == ArgTy::Kind) return static_cast<ArgTy const*>(this);
    else return nullptr;
  }

  friend std::hash<easy::ArgumentBase>;

  virtual ArgumentKind kind() const noexcept = 0;

  protected:
  virtual bool compareWithSameType(ArgumentBase const&) const = 0;
  virtual size_t hash() const noexcept = 0;
};

#define DeclareArgument(Name, Type) \
  class Name##Argument \
    : public ArgumentBase { \
    \
    using HashType = std::remove_const_t<std::remove_reference_t<Type>>; \
    \
    Type Data_; \
    public: \
    Name##Argument(Type D) : ArgumentBase(), Data_(D) {}; \
    virtual ~Name ## Argument() = default; \
    Type get() const { return Data_; } \
    static constexpr ArgumentKind Kind = AK_##Name;\
    ArgumentKind kind() const noexcept override  { return Kind; } \
    \
    protected: \
    bool compareWithSameType(ArgumentBase const& Other) const override { \
      auto const &OtherCast = static_cast<Name##Argument const&>(Other); \
      return Data_ == OtherCast.Data_; \
    } \
    \
    size_t hash() const noexcept override  { return std::hash<HashType>{}(Data_); } \
  }

DeclareArgument(Forward, unsigned);
DeclareArgument(Int, int64_t);
DeclareArgument(Float, double);
DeclareArgument(Ptr, void const*);
DeclareArgument(Module, easy::Function const&);

// NOTE: when more tunable params are added, we probably
// want to make this a macro. The main difference
// between this and DeclareArgument is that we dereference
// the Data_ when doing comparisons.
class IntRangeArgument
    : public ArgumentBase {
  tuned_param::IntRange* Data_;
  public:
  IntRangeArgument(tuned_param::IntRange* D)
    : ArgumentBase(), Data_(D) {};
  virtual ~IntRangeArgument() { delete Data_; }
  tuned_param::IntRange* get() const { return Data_; }
  static constexpr ArgumentKind Kind = AK_IntRange;
  ArgumentKind kind() const noexcept override  { return Kind; }

  protected:
  bool compareWithSameType(ArgumentBase const& Other) const override {
    auto const &OtherCast = static_cast<IntRangeArgument const&>(Other);
    return *Data_ == *OtherCast.Data_;
  }

  size_t hash() const noexcept override {
    return Data_->hash();
  }
};

class StructArgument
    : public ArgumentBase {
  std::vector<char> Data_;
  public:
  StructArgument(const char* Str, size_t Size)
    : ArgumentBase(), Data_(Str, Str+Size) {};
  virtual ~StructArgument() = default;
  std::vector<char> const & get() const { return Data_; }
  static constexpr ArgumentKind Kind = AK_Struct;
  ArgumentKind kind() const noexcept override  { return Kind; }

  protected:
  bool compareWithSameType(ArgumentBase const& Other) const override {
    auto const &OtherCast = static_cast<StructArgument const&>(Other);
    return Data_ == OtherCast.Data_;
  }

  size_t hash() const noexcept override {
    std::hash<int64_t> hash{};
    size_t R = 0;
    for (char c : Data_)
      R ^= hash(c);
    return R;
  }
};

// class that holds information about the just-in-time context
class Context {

  std::vector<std::shared_ptr<ArgumentBase>> ArgumentMapping_;
  unsigned OptLevel_ = 3, OptSize_ = 0;
  std::string DebugFile_;
  std::string DebugBeforeFile_;

  tuner::AutoTuner TunerKind_ = tuner::AT_None;

  // NOTE: these values do not factor into the equality of two contexts,
  // they are blind to these options.
  double FeedbackStdErr_ = DEFAULT_STD_ERR_PCT;
  bool WaitForCompile_ = false;


  template<class ArgTy, class ... Args>
  inline Context& setArg(Args && ... args) {
    ArgumentMapping_.emplace_back(new ArgTy(std::forward<Args>(args)...));
    return *this;
  }

  public:

  Context() = default;

  bool operator==(const Context&) const;

  // set the mapping between
  Context& setParameterIndex(unsigned);
  Context& setParameterInt(int64_t);
  Context& setParameterFloat(double);
  Context& setParameterPointer(void const*);
  Context& setParameterStruct(char const*, size_t);
  Context& setParameterModule(easy::Function const&);
  Context& setTunableParam(tuned_param::IntRange);

  template<class T>
  Context& setParameterTypedPointer(T* ptr) {
    return setParameterPointer(reinterpret_cast<const void*>(ptr));
  }

  template<class T>
  Context& setParameterTypedStruct(T* ptr) {
    return setParameterStruct(reinterpret_cast<char const*>(ptr), sizeof(T));
  }

  Context& setTunerKind(tuner::AutoTuner AT) {
    TunerKind_ = AT;
    return *this;
  }

  Context& setFeedbackStdErr(double val) {
    FeedbackStdErr_ = val;
    return *this;
  }

  double getFeedbackStdErr() const {
    return FeedbackStdErr_;
  }

  Context& setWaitForCompile(bool val) {
    WaitForCompile_ = val;
    return *this;
  }

  bool waitForCompile() const {
    return WaitForCompile_;
  }

  tuner::AutoTuner getTunerKind() const {
    return TunerKind_;
  }

  Context& setOptLevel(unsigned OptLevel, unsigned OptSize) {
    OptLevel_ = OptLevel;
    OptSize_ = OptSize;
    return *this;
  }

  Context& setDebugFile(std::string const &File) {
    DebugFile_ = File;
    return *this;
  }

  Context& setDebugBeforeFile(std::string const &File) {
    DebugBeforeFile_ = File;
    return *this;
  }

  std::pair<unsigned, unsigned> getOptLevel() const {
    return std::make_pair(OptLevel_, OptSize_);
  }

  std::string const& getDebugFile() const {
    return DebugFile_;
  }

  std::string const& getDebugBeforeFile() const {
    return DebugBeforeFile_;
  }

  auto begin() const { return ArgumentMapping_.begin(); }
  auto end() const { return ArgumentMapping_.end(); }
  size_t size() const { return ArgumentMapping_.size(); }

  ArgumentBase const& getArgumentMapping(size_t i) const {
    return *ArgumentMapping_[i];
  }

  friend bool operator<(easy::Context const &C1, easy::Context const &C2);
};

} // end namespace

#include <iostream>

namespace std
{
  template<class L, class R> struct hash<std::pair<L, R>>
  {
    typedef std::pair<L,R> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
      result_type H = std::hash<L>{}(s.first) ^ std::hash<R>{}(s.second);
      return H;
    }
  };

  template<> struct hash<easy::ArgumentBase>
  {
    typedef easy::ArgumentBase argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
      return s.hash();
    }
  };

  template<> struct hash<easy::Context>
  {
    typedef easy::Context argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& C) const noexcept {
      size_t H = (size_t) C.getTunerKind();
      std::hash<easy::ArgumentBase> ArgHash;
      std::hash<std::pair<unsigned, unsigned>> OptHash;
      for(auto const &Arg : C)
        H ^= ArgHash(*Arg);
      H ^= OptHash(C.getOptLevel());
      return H;
    }
  };

  template<> struct hash<std::shared_ptr<easy::Context>>
  {
    typedef std::shared_ptr<easy::Context> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
      result_type H = std::hash<easy::Context>{}(*(s.get()));
      return H;
    }
  };

  template<> struct equal_to<std::shared_ptr<easy::Context>>
  {
    typedef std::shared_ptr<easy::Context> argument_type;
    bool operator()(argument_type const& lhs, argument_type const& rhs) const noexcept {
      // operator== is defined for Contexts
      return *lhs == *rhs;
    }
  };

  template<> struct equal_to<std::pair<void*, std::shared_ptr<easy::Context>>>
  {
    typedef std::pair<void*, std::shared_ptr<easy::Context>> argument_type;
    bool operator()(argument_type const& lhs, argument_type const& rhs) const noexcept {
      std::equal_to<std::shared_ptr<easy::Context>> SharedCxtEqualTo;

      return (lhs.first == rhs.first) && (SharedCxtEqualTo(lhs.second, rhs.second));
    }
  };

} // end namespace std


#endif
