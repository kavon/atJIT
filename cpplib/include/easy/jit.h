#ifndef EASY
#define EASY

#include <easy/runtime/Context.h>
#include <easy/runtime/Function.h>
#include <easy/meta.h>
#include <easy/param.h>

#include <memory>
#include <type_traits>
#include <tuple>
#include <cassert>

namespace easy {

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

  template<class ... Args>
  Ret operator()(Args&& ... args) const {
    return getRawPointer<Ret(Params ...)>(std::forward<Args>(args)...);
  }
};

// specialization for void return
template<class ... Params>
class FunctionWrapper<void, Params ...> :
    public FunctionWrapperBase {
  public:
  FunctionWrapper(std::unique_ptr<Function> F)
    : FunctionWrapperBase(std::move(F)) {}

  template<class ... Args>
  void operator()(Args&& ... args) const {
    getRawPointer<void(Params ...)>(std::forward<Args>(args)...);
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

  using new_type_traits = meta::new_function_traits<FunOriginalTy>;
  using new_return_type = typename new_type_traits::return_type;
  using new_parameter_types = typename new_type_traits::parameter_list;

  static_assert(nargs == nparams,
                "easy::jit: incorrect number of arguments passed.");

  std::unique_ptr<Context> C(new Context(nparams));
  easy::set_parameters(*C, 0, parameter_list(), std::forward<Args>(args)...);

  auto CompiledFunction = Function::Compile(reinterpret_cast<void*>(meta::get_as_pointer(Fun)), std::move(C));

  auto Wrapper =
      FunctionWrapperBase::get(std::move(CompiledFunction),
                               typename new_parameter_types::template push_front<new_return_type> ());

  return Wrapper;
}

}

#endif
