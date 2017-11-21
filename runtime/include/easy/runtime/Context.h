#ifndef CONTEXT
#define CONTEXT

#include <vector>
#include <cstdint>

namespace easy {

struct Argument {
  enum class Type {Arg, Int, Float, Ptr};

  Type ty;
  union {
    unsigned param_idx;
    int64_t integer;
    double floatting;
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
  void setParameterPtr(unsigned, void*);

  private:
  void initDefaultArgumentMapping();
}; 

}

#endif
