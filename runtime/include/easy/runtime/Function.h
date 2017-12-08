#ifndef FUNCTION
#define FUNCTION

#include <easy/runtime/Context.h>
#include <easy/runtime/Function.h>
#include <easy/runtime/BitcodeTracker.h>

namespace easy {

class Function {

  // do not reorder the fields and do not add virtual methods!
  void* Address; 

  std::unique_ptr<llvm::ExecutionEngine> Engine;

  public:

  Function(void* Addr, std::unique_ptr<llvm::ExecutionEngine> EE) 
    : Address(Addr), Engine(std::move(EE)) {
  }

  void* getRawPointer() const {
    return Address;
  }

  static std::unique_ptr<Function> Compile(void *Addr, Context const &C);

  private:

  static std::unique_ptr<llvm::TargetMachine> GetTargetMachine(llvm::StringRef Triple); 
  static void Optimize(llvm::Module& M, const char* Name, easy::Context const& C, unsigned OptLevel = 0, unsigned OptSize = 0);
  static std::unique_ptr<llvm::ExecutionEngine> GetEngine(std::unique_ptr<llvm::Module> M); 
  static void MapGlobals(llvm::ExecutionEngine& EE, GlobalMapping* Globals);
};

}

#endif
