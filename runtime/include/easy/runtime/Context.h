#ifndef CONTEXT
#define CONTEXT

#include <vector>
#include <cstdint>

namespace easy {

struct Argument {
  enum class Type {Forward, Int, Float, Ptr};

  Type ty;
  union {
    unsigned param_idx;
    int64_t integer;
    double floating;
    void* ptr;
  } data;

  bool operator<(Argument const& Other) const {
    if(ty < Other.ty)
      return true;
    if(ty == Other.ty && data.integer < Other.data.integer)
      return true;
    return false;
  }

  bool operator==(Argument const& Other) const {
    return ty == Other.ty && data.integer == Other.data.integer;
  }

  bool operator!=(Argument const& Other) const {
    return !(*this==Other);
  }
};

// class that holds information about the just-in-time context
class Context {

  std::vector<Argument> ArgumentMapping_; 
  unsigned OptLevel_ = 1, OptSize_ = 0;

  public:

  Context(int nargs) 
    : ArgumentMapping_(nargs) { 
    initDefaultArgumentMapping(); 
  }
  
  // set the mapping between 
  Context& setParameterIndex(unsigned, unsigned);
  Context& setParameterInt(unsigned, int64_t);
  Context& setParameterFloat(unsigned, double);

  Context& setParameterPtrVoid(unsigned, void*);

  template<class T>
  Context& setParameterPtr(unsigned idx, T* ptr) {
    return setParameterPtrVoid(idx, reinterpret_cast<void*>(ptr));
  }

  Context& setOptLevel(unsigned OptLevel, unsigned OptSize) {
    OptLevel_ = OptLevel;
    OptSize_ = OptSize;
    return *this;
  }

  std::pair<unsigned, unsigned> getOptLevel() const {
    return std::make_pair(OptLevel_, OptSize_);
  }

  auto begin() const { return ArgumentMapping_.begin(); }
  auto end() const { return ArgumentMapping_.end(); }
  size_t size() const { return ArgumentMapping_.size(); }

  Argument const& getArgumentMapping(size_t i) const {
    return ArgumentMapping_[i];
  }

  friend bool operator<(easy::Context const &C1, easy::Context const &C2);

  private:
  void initDefaultArgumentMapping();
}; 

bool operator<(easy::Context const &C1, easy::Context const &C2);

}

#endif
