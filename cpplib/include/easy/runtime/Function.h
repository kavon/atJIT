#ifndef FUNCTION
#define FUNCTION

#include "Context.h"
#include "LLVMHolder.h"

namespace easy {

class GlobalMapping;

class Function {

  // do not reorder the fields and do not add virtual methods!
  void* Address; 
  std::unique_ptr<easy::LLVMHolder> Holder;

  public:

  Function(void* Addr, std::unique_ptr<easy::LLVMHolder> H);

  void* getRawPointer() const {
    return Address;
  }

  static std::unique_ptr<Function> Compile(void *Addr, easy::Context const &C);
};

}

#endif
