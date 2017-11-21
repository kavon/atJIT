#ifndef EASY
#define EASY

#include <easy/runtime/CompilerInterface.h>
#include <memory>
#include <cassert>

namespace easy {
  
class FunctionWrapperBase {

  protected:
  std::unique_ptr<Function> Fun;

  public:
  FunctionWrapperBase(Function* F)
    : Fun(F) {}
};

template<class FunTy>
class FunctionWrapper : public FunctionWrapperBase {

  public:
  FunctionWrapper(Function* F)
    : FunctionWrapperBase(F) { }

  // TODO operator()
};

template<class Val>
void set_single_parameter(Context &, size_t, Val&&) {
  assert(false && "dont know what to do!");
}

inline void set_parameters(Context&, size_t){ return; }

template<class Arg0, class ... Args>
inline void set_parameters(Context &C, size_t idx, Arg0&& arg0, Args&& ... args) {
  set_single_parameter(C, idx, std::forward(arg0));
  set_parameters(C, idx+1, std::forward(args...));
}

template<class FunTy, class ... Args>
FunctionWrapper</*TODO the type depends on the args*/> jit(FunTy &&Fun, Args&& ... args) {
  constexpr size_t nargs = sizeof...(Args);
  Context *C = easy_new_context(nargs);
  set_parameters<Args...>(*C, 0, std::forward(args...));
  FunctionWrapper</*TODO the type depends on the args*/> Wrap(easy_generate_code((void*)Fun, C));
  return Wrap;
}

}

#endif
