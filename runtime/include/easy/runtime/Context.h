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
};

// class that holds information about the just-in-time context
class Context {

  std::vector<Argument> ArgumentMapping_; 

  public:

  Context(int nargs) 
    : ArgumentMapping_(nargs) { 
    initDefaultArgumentMapping(); 
  }
  
  // set the mapping between 
  void setParameterIndex(unsigned, unsigned); 
  void setParameterInt(unsigned, int64_t);
  void setParameterFloat(unsigned, double); 

  void setParameterPtrVoid(unsigned, void*);

  template<class T>
  void setParameterPtr(unsigned idx, T* ptr) {
    setParameterPtrVoid(idx, reinterpret_cast<void*>(ptr));
  }

  auto begin() const { return ArgumentMapping_.begin(); }
  auto end() const { return ArgumentMapping_.end(); }
  size_t size() const { return ArgumentMapping_.size(); }

  Argument const& getArgumentMapping(size_t i) const {
    return ArgumentMapping_[i];
  }


  private:
  void initDefaultArgumentMapping();
}; 

}

#endif
