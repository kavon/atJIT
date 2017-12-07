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

class FunctionWrapperBase {

  protected:
  std::unique_ptr<Function> Fun_;

  public:
  // null object
  FunctionWrapperBase() = default;

  // default constructor
  FunctionWrapperBase(std::unique_ptr<Function> F)
    : Fun_(std::move(F)) {}

  // steal the implementation
  FunctionWrapperBase(FunctionWrapperBase &&FW)
    : Fun_(std::move(FW.Fun_)) {}
  FunctionWrapperBase& operator=(FunctionWrapperBase &&FW) {
    Fun_ = std::move(FW.Fun_);
    return *this;
  }

  void* getRawPointer() const {
    return Fun_->getRawPointer();
  } 
};

template<class FTy>
class FunctionWrapper;

template<class Ret, class ... Params>
class FunctionWrapper<Ret(Params...)> :
    public FunctionWrapperBase {
  public:
  FunctionWrapper(std::unique_ptr<Function> F)
    : FunctionWrapperBase(std::move(F)) {}

  template<class ... Args>
  Ret operator()(Args&& ... args) const {
    return ((Ret(*)(Params...))getRawPointer())(std::forward<Args>(args)...);
  }
};

// specialization for void return
template<class ... Params>
class FunctionWrapper<void(Params...)> :
    public FunctionWrapperBase {
  public:
  FunctionWrapper(std::unique_ptr<Function> F)
    : FunctionWrapperBase(std::move(F)) {}

  template<class ... Args>
  void operator()(Args&& ... args) const {
    return ((void(*)(Params...))getRawPointer())(std::forward<Args>(args)...);
  }
};

namespace {
template<class Ret, class ... Params>
FunctionWrapper<Ret(Params ...)>
WrapFunction(std::unique_ptr<Function> F, meta::type_list<Ret, Params ...>) {
  return FunctionWrapper<Ret(Params ...)>(std::move(F));
}

template<class T, class ... Args>
auto jit_with_context(std::unique_ptr<easy::Context> C, T &&Fun) {

  auto* FunPtr = meta::get_as_pointer(Fun);
  using FunOriginalTy = typename std::remove_pointer<typename std::decay<T>::type>::type;

  using new_type_traits = meta::new_function_traits<FunOriginalTy, meta::type_list<Args...>>;
  using new_return_type = typename new_type_traits::return_type;
  using new_parameter_types = typename new_type_traits::parameter_list;

  auto CompiledFunction =
      Function::Compile(reinterpret_cast<void*>(FunPtr), std::move(C));

  auto Wrapper =
      WrapFunction(std::move(CompiledFunction),
                   typename new_parameter_types::template push_front<new_return_type> ());
  return Wrapper;
}

template<class T, class ... Args>
std::unique_ptr<Context> get_context_for(Args&& ... args) {
  using FunOriginalTy = typename std::remove_pointer<typename std::decay<T>::type>::type;
  static_assert(std::is_function<FunOriginalTy>::value,
                "easy::jit: supports only on functions and function pointers");

  using parameter_list = typename meta::function_traits<FunOriginalTy>::parameter_list;
  constexpr size_t nparams = parameter_list::size;

  std::unique_ptr<Context> C(new Context(nparams));
  easy::set_parameters<parameter_list, Args&&...>(parameter_list(), *C, 0,
                                                  std::forward<Args>(args)...);
  return C;
}
}

template<class T, class ... Args>
auto jit(T &&Fun, Args&& ... args) {
  auto C = get_context_for<T, Args...>(std::forward<Args>(args)...);
  return jit_with_context<T, Args...>(std::move(C), std::forward<T>(Fun));
}

}

#endif
