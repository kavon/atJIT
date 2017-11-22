#ifndef EASY
#define EASY

#include <easy/runtime/CompilerInterface.h>
#include <easy/meta.h>

#include <memory>
#include <type_traits>
#include <tuple>
#include <cassert>

namespace easy {

class FunctionWrapperBase {

  protected:
  std::unique_ptr<Function> Fun;

  public:
  FunctionWrapperBase(Function* F)
    : Fun(F) {}
};

namespace {

template<class Param0, class Arg0>
void set_parameter<Param0, Arg0>(Context &C, size_t idx, Arg0 &&arg0) {
  // TODO 
  // use a structure and enable_if to specialize according to Param. 
  // type checking will notify if the expected and received are not compatible 
}

template<>
void set_parameters<>(Context &C, size_t idx, meta::type_list<>) {}

template<class ... Parameters, class Arg0, class ... Args>
void set_parameters(Context &C, size_t idx, meta::type_list<Parameters...>, Arg0 &&arg0, Args&& ... args) {
  using Param0 = typename meta::type_list<Parameters...>::head;
  using ParametersTail = typename meta::type_list<Parameters...>::tail;

  set_parameter<Param0, Arg0>(C, idx, std::foward(arg0));
  set_parameters(C, idx+1,  ParametersTail(), std::foward(args...));
}

}

template<class T, class ... Args>
void jit(T &&Fun, Args&& ... args) {
  using FunOriginalTy = typename std::decay<T>::type;
  static_assert(std::is_function<FunOriginalTy>::value,
                "easy::jit: supports only on functions and function pointers");

  using return_type = typename meta::function_traits<FunOriginalTy>::return_type;
  using parameter_list = typename meta::function_traits<FunOriginalTy>::parameter_list;
  using argument_list = meta::type_list<Args&&...>;

  constexpr size_t nargs = argument_list::size;
  constexpr size_t nparams = parameter_list::size;

  static_assert(nargs == nparams,
                "easy::jit: incorrect number of arguments passed.");

  set_parameters(*C, 0, parameter_list(), std::foward(args ...));

  Context *C = easy_new_context(nparams);
}

}

#endif
