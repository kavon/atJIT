#include "easy/runtime/CompilerInterface.h"
#include <cstdio>

using namespace easy;

extern "C" 
Context* easy_new_context(unsigned nargs) {
  return new Context(nargs);
}

extern "C" 
void easy_set_parameter(Context*, unsigned, ...) {
  fprintf(stderr, "easy_set_parameter called. This function should be never called!\n");
} 

extern "C" 
void easy_set_parameter_index(Context* C, unsigned arg, unsigned idx) {
  C->setParameterIndex(arg, idx);
}

extern "C" 
void easy_set_parameter_int(Context* C, unsigned arg, int64_t val) {
  C->setParameterInt(arg, val);
}

extern "C" 
void easy_set_parameter_float(Context* C, unsigned arg, double val) {
  C->setParameterFloat(arg, val);
}

extern "C"
void easy_set_parameter_ptr(Context* C, unsigned arg, void* val) {
  C->setParameterPtr(arg, val);
}

extern "C"
void easy_register(void* addr, const char* name, GlobalMapping* globals, const char* bitcode, size_t bitcode_len) {
  BitcodeTracker::GetTracker().registerFunction(addr, name, globals, bitcode, bitcode_len);
}

extern "C"
Function* easy_generate_code(void* addr, Context* C) {

  const char* Name;
  GlobalMapping* GM;

  auto &BT = BitcodeTracker::GetTracker();
  std::tie(Name, GM) = BT.getNameAndGlobalMapping(addr);
  auto M = BT.getModule(addr);

  auto F = Function::Compile(Name, GM, M, std::unique_ptr<Context>(C));
  return F.release(); // release ownership of the pointer!
}

extern "C"
void easy_destroy(Function* F) {
  delete F;
}
