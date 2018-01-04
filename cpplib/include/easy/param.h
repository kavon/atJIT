#ifndef PARAM
#define PARAM

#include <easy/runtime/Context.h>
#include <easy/options.h>
#include <easy/meta.h>

namespace easy {

namespace  {

template<bool is_placeholder>
struct set_parameter_helper {

  template<class _, class Arg>
  static void set_param(Context &C, size_t idx, Arg &&) {
    C.setParameterIndex(idx, std::is_placeholder<typename std::decay<Arg>::type>::value-1);
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
    C.setParameterPtr(idx, std::addressof(arg));
  }

  template<class Param, class Arg>
  static void set_param(Context &C, size_t idx,
                        enable_if<std::is_class<Param>::value, Arg> &&arg) {
    C.setParameterStruct(idx, std::addressof(arg));
  }
};

template<class Param, class Arg>
struct set_parameter :
    public set_parameter_helper<
             (bool)std::is_placeholder<typename std::decay<Arg>::type>::value> {
};

}

template<class ... NoOptions>
void set_options(Context &, NoOptions&& ...) {
  static_assert(meta::type_list<NoOptions...>::empty, "Remaining options to be processed!");
}

template<class Option0, class ... Options>
void set_options(Context &C, Option0&& Opt, Options&& ... Opts) {
  using OptTy = typename std::decay<Option0>::type;
  OptTy& OptRef = std::ref<OptTy>(Opt);
  static_assert(options::is_option<OptTy>::value, "An easy::jit option is expected");

  OptRef.handle(C);
  set_options(C, std::forward<Options>(Opts)...);
}

template<class ParameterList, class ... Options>
typename std::enable_if<ParameterList::empty>::type
set_parameters(ParameterList,
               Context& C, size_t, Options&& ... opts) {
  set_options<Options...>(C, std::forward<Options>(opts)...);
}

template<class ParameterList, class Arg0, class ... Args>
typename std::enable_if<!ParameterList::empty>::type
set_parameters(ParameterList,
               Context &C, size_t idx, Arg0 &&arg0, Args&& ... args) {
  using Param0 = typename ParameterList::head;
  using ParametersTail = typename ParameterList::tail;

  set_parameter<Param0, Arg0>::template set_param<Param0, Arg0>(C, idx, std::forward<Arg0>(arg0));
  set_parameters<ParametersTail, Args&&...>(ParametersTail(), C, idx+1, std::forward<Args>(args)...);
}

}

#endif // PARAM
