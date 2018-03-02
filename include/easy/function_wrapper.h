#ifndef FUNCTION_WRAPPER
#define FUNCTION_WRAPPER

#include <iostream>
#include <memory>
#include <easy/runtime/Function.h>
#include <easy/meta.h>

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

  Function const& getFunction() const {
    return *Fun_;
  }

  void* getRawPointer() const {
    return getFunction().getRawPointer();
  }

  void serialize(std::ostream& os) const {
    getFunction().serialize(os);
  }

  static FunctionWrapperBase deserialize(std::istream& is) {
    std::unique_ptr<Function> Fun = Function::deserialize(is);
    return FunctionWrapperBase{std::move(Fun)};
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

  static FunctionWrapper<Ret(Params...)> deserialize(std::istream& is) {
    std::unique_ptr<Function> Fun = Function::deserialize(is);
    return FunctionWrapper<Ret(Params...)>{std::move(Fun)};
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

  static FunctionWrapper<void(Params...)> deserialize(std::istream& is) {
    std::unique_ptr<Function> Fun = Function::deserialize(is);
    return FunctionWrapper<void(Params...)>{std::move(Fun)};
  }
};

template<class T>
struct is_function_wrapper {

  template<class _>
  struct is_function_wrapper_helper {
    static constexpr bool value = false;
  };

  template<class Ret, class ... Params>
  struct is_function_wrapper_helper<FunctionWrapper<Ret(Params...)>> {
    static constexpr bool value = true;
    using return_type = Ret;
    using params = meta::type_list<Params...>;
  };

  using helper = is_function_wrapper_helper<typename std::remove_reference<T>::type>;

  static constexpr bool value = helper::value;
};

template<class Ret, class ... Params>
struct is_function_wrapper<FunctionWrapper<Ret(Params...)>> {
  static constexpr bool value = true;
  using return_type = Ret;
  using params = meta::type_list<Params...>;
};


}

#endif
