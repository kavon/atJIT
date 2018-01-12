#ifndef CONTEXT
#define CONTEXT

#include <vector>
#include <memory>
#include <cstdint>
#include <cstring>

namespace easy {

struct ArgumentBase {
  ArgumentBase() = default;
  virtual ~ArgumentBase() = default;

  bool operator==(ArgumentBase const &Other) const {
    return typeid(*this) == typeid(Other) &&
        this->compareWithSameType(Other);
  }

  template<class ArgTy>
  typename std::enable_if<std::is_base_of<ArgumentBase, ArgTy>::value, ArgTy const*>::type
  as() const {
    return dynamic_cast<ArgTy const*>(this);
  }

  friend std::hash<easy::ArgumentBase>;

  protected:
  virtual bool compareWithSameType(ArgumentBase const&) const = 0;
  virtual size_t hash() const noexcept = 0;
};

#define DeclareArgument(Name, Type) \
  class Name##Argument \
    : public ArgumentBase { \
    Type Data_; \
    public: \
    Name##Argument(Type D) : ArgumentBase(), Data_(D) {}; \
    virtual ~Name ## Argument() = default; \
    Type get() const { return Data_; } \
    \
    protected: \
    bool compareWithSameType(ArgumentBase const& Other) const override { \
      auto const &OtherCast = static_cast<Name##Argument const&>(Other); \
      return Data_ == OtherCast.Data_; \
    } \
    \
    size_t hash() const noexcept override  { return std::hash<Type>{}(Data_); } \
  }

DeclareArgument(Forward, unsigned);
DeclareArgument(Int, int64_t);
DeclareArgument(Float, double);
DeclareArgument(Ptr, void const*);

class StructArgument
    : public ArgumentBase {
  std::vector<char> Data_;
  public:
  StructArgument(const char* Str, size_t Size)
    : ArgumentBase(), Data_(Str, Str+Size) {};
  virtual ~StructArgument() = default;
  std::vector<char> const & get() const { return Data_; }

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

  std::vector<std::unique_ptr<ArgumentBase>> ArgumentMapping_;
  unsigned OptLevel_ = 2, OptSize_ = 0;
  std::string DebugFile_;

  template<class ArgTy, class ... Args>
  inline Context& setArg(size_t i, Args && ... args) {
    ArgumentMapping_[i] =
        std::unique_ptr<ArgumentBase>(new ArgTy(std::forward<Args>(args)...));
    return *this;
  }

  public:

  Context(int nargs) 
    : ArgumentMapping_(nargs) { 
    initDefaultArgumentMapping(); 
  }

  bool operator==(const Context&) const;
  
  // set the mapping between
  Context& setParameterIndex(unsigned, unsigned);
  Context& setParameterInt(unsigned, int64_t);
  Context& setParameterFloat(unsigned, double);
  Context& setParameterPtrVoid(unsigned, void const*);
  Context& setParameterPlainStruct(unsigned, char const*, size_t);

  template<class T>
  Context& setParameterPtr(unsigned idx, T* ptr) {
    return setParameterPtrVoid(idx, reinterpret_cast<void*>(ptr));
  }
  template<class T>
  Context& setParameterPtr(unsigned idx, T const* ptr) {
    return setParameterPtrVoid(idx, reinterpret_cast<void const*>(ptr));
  }

  template<class T>
  Context& setParameterStruct(unsigned idx, T* ptr) {
    return setParameterPlainStruct(idx, reinterpret_cast<char const*>(ptr), sizeof(T));
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

  std::pair<unsigned, unsigned> getOptLevel() const {
    return std::make_pair(OptLevel_, OptSize_);
  }

  std::string const& getDebugFile() const {
    return DebugFile_;
  }

  auto begin() const { return ArgumentMapping_.begin(); }
  auto end() const { return ArgumentMapping_.end(); }
  size_t size() const { return ArgumentMapping_.size(); }

  ArgumentBase const& getArgumentMapping(size_t i) const {
    return *ArgumentMapping_[i];
  }

  friend bool operator<(easy::Context const &C1, easy::Context const &C2);

  private:
  void initDefaultArgumentMapping();
}; 

}

namespace std
{
  template<class L, class R> struct hash<std::pair<L, R>>
  {
    typedef std::pair<L,R> argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
      return std::hash<L>{}(s.first) ^ std::hash<R>{}(s.second);
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
      size_t H = 0;
      std::hash<easy::ArgumentBase> ArgHash;
      std::hash<std::pair<unsigned, unsigned>> OptHash;
      for(auto const &Arg : C)
        H ^= ArgHash(*Arg);
      H ^= OptHash(C.getOptLevel());
      return H;
    }
  };
}


#endif
