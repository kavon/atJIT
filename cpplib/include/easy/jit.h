#ifndef EASY
#define EASY

#include <easy/runtime/Context.h>
#include <easy/runtime/Function.h>
#include <easy/meta.h>

#include <memory>
#include <type_traits>
#include <tuple>
#include <cassert>

namespace easy {


namespace {

template<bool is_placeholder>
struct set_parameter_helper {

  template<class _, class Arg>
  static void set_param(Context &C, size_t idx, Arg &&) {
    C.setParameterIndex(idx, std::is_placeholder<Arg>::value);
  }
};

template<>
struct set_parameter_helper<false> {

  template<bool B, class T>
  using enable_if = typename std::enable_if<B, T>::type;

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        enable_if<std::is_integral<Param>::value, Arg> &&arg) {
    C.setParameterInt(idx, std::forward<Arg>(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        enable_if<std::is_floating_point<Param>::value, Arg> &&arg) {
    C.setParameterFloat(idx, std::forward<Arg>(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        enable_if<std::is_pointer<Param>::value, Arg> &&arg) {
    C.setParameterPtr(idx, std::forward<Arg>(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        enable_if<std::is_reference<Param>::value, Arg> &&arg) {
    C.setParameterPtr(idx, std::addressof(std::forward<Arg>(arg)));
  }
};

template<class Param, class Arg>
struct set_parameter :
    public set_parameter_helper<
             std::is_placeholder<typename std::decay<Arg>::type>::value> {
};

template<class ParameterList>
void set_parameters(Context&, size_t, ParameterList) {
  static_assert(ParameterList::empty, 
                "easy::jit: there are more parameters than arguments passed to the function.");
}

template<class ParameterList, class Arg0, class ... Args>
void set_parameters(Context &C, size_t idx, ParameterList, Arg0 &&arg0, Args&& ... args) {
  using Param0 = typename ParameterList::head;
  using ParametersTail = typename ParameterList::tail;

  set_parameter<Param0, Arg0>::template set_param<Param0, Arg0>(C, idx, std::forward<Arg0>(arg0));
  set_parameters(C, idx+1,  ParametersTail(), std::forward<Args>(args)...);
}

}

template<class Ret, class ... Params>
class FunctionWrapper;

class FunctionWrapperBase {

  protected:
  std::unique_ptr<Function> Fun_;

  public:
  FunctionWrapperBase(std::unique_ptr<Function> F)
    : Fun_(std::move(F)) {}

  template<class F>
  F* getRawPointer() const {
    return reinterpret_cast<F*>(Fun_->getRawPointer());
  }

  template<class Ret, class ... Params>
  static
  FunctionWrapper<Ret, Params ...>
  get(std::unique_ptr<Function> F, meta::type_list<Ret, Params ...>) {
    return FunctionWrapper<Ret, Params ...>(std::move(F));
  }
};

template<class Ret, class ... Params>
class FunctionWrapper :
    public FunctionWrapperBase {

  public:

  FunctionWrapper(std::unique_ptr<Function> F)
    : FunctionWrapperBase(std::move(F)) {}

  Ret operator()(Params&& ... params) const {
    return getRawPointer<Ret(Params ...)>(std::forward<Params>(params)...);
  }
};

template<class T, class ... Args>
auto jit(T &&Fun, Args&& ... args) {
  auto* FunPtr = meta::get_as_pointer(Fun);
  using FunOriginalTy = typename std::remove_pointer<typename std::decay<decltype(FunPtr)>::type>::type;
  static_assert(std::is_function<FunOriginalTy>::value,
                "easy::jit: supports only on functions and function pointers");

  using parameter_list = typename meta::function_traits<FunOriginalTy>::parameter_list;
  using argument_list = meta::type_list<Args&&...>;

  constexpr size_t nargs = argument_list::size;
  constexpr size_t nparams = parameter_list::size;

  static_assert(nargs == nparams,
                "easy::jit: incorrect number of arguments passed.");

  std::unique_ptr<Context> C(new Context(nparams));
  set_parameters(*C, 0, parameter_list(), std::forward<Args>(args)...);

  using new_type_traits = meta::new_function_traits<FunOriginalTy>;
  using new_return_type = typename new_type_traits::return_type;
  using new_parameter_types = typename new_type_traits::parameter_list;
  auto JitFun = FunctionWrapperBase::get(
                  Function::Compile((void*)meta::get_as_pointer(Fun), std::move(C)),
                  typename new_parameter_types::template push_front<new_return_type> ());

  return JitFun;
}

}

#endif
