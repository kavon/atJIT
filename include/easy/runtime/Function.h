#ifndef FUNCTION
#define FUNCTION

#include <memory>

#include <easy/runtime/LLVMHolder.h>

#include <llvm/Support/CodeGen.h>

// NOTE(kavon): everything somehow breaks if you try to include any
// type definitions here. you need to forward-declare anything
// you need here instead.

namespace easy {
  class Function;
}

namespace llvm {
  class Module;
  class LLVMContext;
}

namespace tuner {
  class Optimizer;
  class Feedback;
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
struct GlobalMapping;

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

  static std::unique_ptr<Function> CompileAndWrap (
    const char*Name, GlobalMapping* Globals,
     std::unique_ptr<llvm::LLVMContext> LLVMCxt,
     std::unique_ptr<llvm::Module> M,
     llvm::CodeGenOpt::Level CGLevel
  );

  static void WriteOptimizedToFile(llvm::Module const &M, std::string const& File);

  friend
  std::hash<easy::Function>::result_type std::hash<easy::Function>::operator()(argument_type const& F) const noexcept;
};

}

#endif
