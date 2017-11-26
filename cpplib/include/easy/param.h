#ifndef PARAM
#define PARAM

#include <easy/runtime/Context.h>
#include <easy/meta.h>

namespace easy {

namespace  {

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

}

template<class ParameterList, class Arg0, class ... Args>
void set_parameters(Context &C, size_t idx, ParameterList, Arg0 &&arg0, Args&& ... args) {
  using Param0 = typename ParameterList::head;
  using ParametersTail = typename ParameterList::tail;

  set_parameter<Param0, Arg0>::template set_param<Param0, Arg0>(C, idx, std::forward<Arg0>(arg0));
  set_parameters(C, idx+1,  ParametersTail(), std::forward<Args>(args)...);
}

}

#endif // PARAM
