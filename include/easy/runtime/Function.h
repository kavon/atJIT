#ifndef FUNCTION
#define FUNCTION

#include "LLVMHolder.h"
#include <memory>

namespace easy {
  class Function;
}

namespace llvm {
  class Module;
}

namespace std {
  template<> struct hash<easy::Function>
  {
    typedef easy::Function argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& F) const noexcept;
  };
}

namespace easy {

class Context;
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

  void serialize(std::ostream&) const;
  static std::unique_ptr<Function> deserialize(std::istream&);

  bool operator==(easy::Function const&) const;

  llvm::Module const& getLLVMModule() const;

  static std::unique_ptr<Function> Compile(void *Addr, easy::Context const &C);

  friend
  std::hash<easy::Function>::result_type std::hash<easy::Function>::operator()(argument_type const& F) const noexcept;
};

}

#endif
