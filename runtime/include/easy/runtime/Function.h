#ifndef FUNCTION
#define FUNCTION

#include "Context.h"
#include "BitcodeTracker.h"
#include "LLVMHolder.h"

namespace easy {

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

  private:

  static std::unique_ptr<llvm::TargetMachine> GetHostTargetMachine();
  static void Optimize(llvm::Module& M, const char* Name, easy::Context const& C, unsigned OptLevel = 0, unsigned OptSize = 0);
  static std::unique_ptr<llvm::ExecutionEngine> GetEngine(std::unique_ptr<llvm::Module> M, const char *Name);
  static void MapGlobals(llvm::ExecutionEngine& EE, GlobalMapping* Globals);
  static void WriteOptimizedToFile(llvm::Module const &M, std::string const& File);
};

}

#endif
