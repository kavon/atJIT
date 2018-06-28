#ifndef EASY
#define EASY

#include <easy/runtime/Context.h>
#include <easy/attributes.h>
#include <easy/param.h>
#include <easy/function_wrapper.h>

#include <tuner/optimizer.h>

#include <memory>
#include <type_traits>
#include <tuple>
#include <cassert>

namespace tuner {
  class Optimizer;
}

namespace easy {

namespace {
template<class Ret, class ... Params>
FunctionWrapper<Ret(Params ...)>
WrapFunction(std::unique_ptr<Function> F, meta::type_list<Ret, Params ...>) {
  return FunctionWrapper<Ret(Params ...)>(std::move(F));
}

template<class T, class ... Args>
auto jit_with_optimizer(tuner::Optimizer &Opt, T &&Fun) {

  auto* FunPtr = meta::get_as_pointer(Fun);
  using FunOriginalTy = std::remove_pointer_t<std::decay_t<T>>;

  using new_type_traits = meta::new_function_traits<FunOriginalTy, meta::type_list<Args...>>;
  using new_return_type = typename new_type_traits::return_type;
  using new_parameter_types = typename new_type_traits::parameter_list;

  auto CompiledFunction =
      Function::Compile(Opt);

  auto Wrapper =
      WrapFunction(std::move(CompiledFunction),
                   typename new_parameter_types::template push_front<new_return_type> ());
  return Wrapper;
}

template<class T, class ... Args>
auto jit_with_context(easy::Context const& Cxt, T &&Fun) {

  auto* FunPtr = meta::get_as_pointer(Fun);
  tuner::Optimizer Opt(reinterpret_cast<void*>(FunPtr), Cxt);

  return jit_with_optimizer<T, Args...>(Opt, std::forward<T>(Fun));
}

template<class T, class ... Args>
easy::Context get_context_for(Args&& ... args) {
  using FunOriginalTy = std::remove_pointer_t<std::decay_t<T>>;
  static_assert(std::is_function<FunOriginalTy>::value,
                "easy::jit: supports only on functions and function pointers");

  using parameter_list = typename meta::function_traits<FunOriginalTy>::parameter_list;

  static_assert(parameter_list::size <= sizeof...(Args),
                "easy::jit: not providing enough argument to actual call");

  easy::Context C;
  easy::set_parameters<parameter_list, Args&&...>(parameter_list(), C,
                                                  std::forward<Args>(args)...);
  return C;
}
}

template<class T, class ... Args>
auto EASY_JIT_COMPILER_INTERFACE jit(T &&Fun, Args&& ... args) {
  auto C = get_context_for<T, Args...>(std::forward<Args>(args)...);
  return jit_with_context<T, Args...>(C, std::forward<T>(Fun));
}

}

#endif
