#ifndef META
#define META

#include <functional>
#include <type_traits>
#include <cstddef>
#include <easy/options.h>

namespace easy {
namespace meta {

template<class ... Empty>
struct type_list {

  template<class New>
  using push_front = struct type_list<New>;
  template<class New>
  using push_back = struct type_list<New>;
  template<class _>
  using remove = type_list<>;
  template<class _>
  static constexpr bool has = false;

  // undefined
  template<size_t _>
  using at = void;
  template<size_t _, class T>
  using set = meta::type_list<>;

  static constexpr size_t size = 0;
  static constexpr bool empty = true;
};

template<class Head, class ... Tail> 
struct type_list<Head, Tail ...> {
  using head = Head;
  using tail = struct type_list<Tail...>;

  template<class New>
  using push_front = struct type_list<New, Head, Tail ...>;
  template<class New>
  using push_back = struct type_list<Head, Tail ..., New>;
  template<size_t I>
  using at = std::conditional_t<I==0, head, typename tail::template at<I-1>>;

  template<size_t I, class T>
  using set = std::conditional_t<I==0,
                                 typename tail::template push_front<T>,
                                 typename tail::template set<I-1, T>::template push_front<head>>;

  template<class T>
  using remove = std::conditional_t<std::is_same<head, T>::value,
                                    typename tail::template remove<T>,
                                    typename tail::template remove<T>::template push_front<head>>;
  template<class T>
  static constexpr bool has = std::is_same<T, head>::value || tail:: template has<T>;

  static constexpr size_t size = 1+tail::size;
  static constexpr bool empty = false;
};

template<size_t N, class T>
struct init_list {

  template<size_t NN, class TT>
  struct helper {
    using type = typename helper<NN-1, TT>::type::template push_front<TT>;
  };

  template<class TT>
  struct helper<0,TT> {
    using type = meta::type_list<>;
  };

  using type = typename helper<N, T>::type;
};

template<class T>
T* get_as_pointer(T &&A) { return &A; }

template<class T>
T* get_as_pointer(T* A) { return A; }


namespace  {

template<class T>
using is_ph = std::is_placeholder<std::decay_t<T>>;

template<class ArgList>
struct discard_options {

  template<class AL, bool Empty>
  struct helper {
    using type = meta::type_list<>;
  };

  template<class AL>
  struct helper<AL, false> {
    using head = typename AL::head;
    using tail = typename AL::tail;
    static bool constexpr is_opt =
        options::is_option<std::decay_t<head>>::value;
    using recursive = typename helper<tail, tail::empty>::type;
    using type = std::conditional_t<is_opt,
                                    recursive,
                                    typename recursive::template push_front<head>>;
  };

  using type = typename helper<ArgList, ArgList::empty>::type;
};

template<class ArgList>
struct max_placeholder {

  template<class AL, bool Empty, size_t Max>
  struct helper {
    static constexpr size_t max = Max;
  };

  template<class AL, size_t Max>
  struct helper<AL, false, Max> {
    using head = typename AL::head;
    using tail = typename AL::tail;
    static constexpr size_t max = helper<tail, tail::empty, std::max<size_t>(is_ph<head>::value, Max)>::max;
  };

  static constexpr size_t max = helper<ArgList, ArgList::empty, 0>::max;
};

template<class ParamList, class ArgList>
struct map_placeholder_to_type {

  template<class _, class __, class Result, class Seen, size_t N, bool Done>
  struct helper {
    static_assert(Seen::size == N, "Seen::size != N");
    static_assert(Result::size == N, "Result::size != N");
    static_assert(!Result::template has<void>, "Void cannot appear in the resulting type");
    using type = Result;
  };

  template<class PL, class AL, class Result, class Seen, size_t N>
  struct helper<PL, AL, Result, Seen, N, false>{
    using al_head = typename AL::head;
    using al_tail = typename AL::tail;

    static bool constexpr parse_placeholder = is_ph<al_head>::value && !Seen::template has<al_head>;
    static size_t constexpr result_idx = parse_placeholder?is_ph<al_head>::value-1:0;
    static size_t constexpr arg_idx = PL::size - AL::size;
    using pl_at_idx = typename PL::template at<arg_idx>;

    // for
    // foo(int, bool, float) and specialization foo(int(4), _2, _1)
    // [int,bool,float] [int,_2,_1] [void,void] [] 2
    // [int,bool,float] [_2,_1] [void,void] [] 2
    // [int,bool,float] [_1] [void,bool] [_2] 2
    // [int,bool,float] [] [float,bool] [_2,_1] 2
    // yields new foo'(float _1, bool _2) = foo(int(4), _2, _1);

    static_assert(PL::size >= AL::size, "easy::jit: More parameters than arguments specified");
    static_assert(result_idx < Result::size, "easy::jit: Cannot have a placeholder outside the maximum");
    using new_result = std::conditional_t<parse_placeholder, typename Result::template set<result_idx, pl_at_idx>, Result>;
    using new_seen = std::conditional_t<parse_placeholder, typename Seen::template push_back<al_head>, Seen>;

    using type = typename helper<PL, al_tail, new_result, new_seen, N, new_seen::size == N>::type;
  };
  template<class PL, class Result, class Seen, size_t N>
  struct helper<PL, type_list<>, Result, Seen, N, false>{
    static_assert(N==-1 /*just to make this context dependent*/, "easy::jit: Invalid bind, placeholder cannot be bound to a formal argument");
  };

  using default_param = void;
  static constexpr size_t N = max_placeholder<ArgList>::max;
  using type = typename helper<ParamList, ArgList, typename init_list<N, default_param>::type, meta::type_list<>, N, N == 0>::type;
};

template<class ParamList, class ArgList>
struct get_new_param_list {
  using type = typename map_placeholder_to_type<ParamList, ArgList>::type;
};

template<class Ret, class ... Args>
Ret get_return_type(Ret(*)(Args...));

template<class Ret, class ... Args>
type_list<Args...> get_parameter_list(Ret(*)(Args...));

}

template<class FunTy>
struct function_traits {
  static_assert(std::is_function<FunTy>::value, "function expected.");
  using ptr_ty = std::decay_t<FunTy>;
  using return_type = decltype(get_return_type(std::declval<ptr_ty>()));
  using parameter_list = decltype(get_parameter_list(std::declval<ptr_ty>()));
};

template<class FunTy, class ArgList>
struct new_function_traits {
  using OriginalTraits = function_traits<FunTy>;
  using return_type = typename OriginalTraits::return_type;
  using clean_arg_list = typename discard_options<ArgList>::type;
  using parameter_list = typename get_new_param_list<typename OriginalTraits::parameter_list, clean_arg_list>::type;
};

}
}



#endif
