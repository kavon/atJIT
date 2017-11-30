#ifndef META
#define META

#include <functional>
#include <type_traits>
#include <cstddef>

namespace easy {
namespace meta {

template<class ... T>
struct type_list {

  template<class New>
  using push_front = struct type_list<New>;
  template<class New>
  using push_back = struct type_list<New>;

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

  static constexpr size_t size = 1+tail::size;
  static constexpr bool empty = false;
};

template<class T>
T* get_as_pointer(T &&A) { return &A; }

template<class T>
T* get_as_pointer(T* A) { return A; }


namespace  {

template<bool Empty>
struct remove_placeholders_helper {
  template<class EmptyList, class ArgList>
  using type = struct type_list<>;
};

template<>
struct remove_placeholders_helper<false> {

  template<class ParamList, class ArgList>
  struct __helper {
    static_assert(!ParamList::empty, "remove_placeholders_helper: type list is not empty!");

    using head = typename ParamList::head;
    using arg_head = typename ArgList::head;
    using tail = typename ParamList::tail;
    using arg_tail = typename ArgList::tail;
    using recursive = typename remove_placeholders_helper<tail::empty>::template type<tail, arg_tail>;
    using keep_head_recursive = typename recursive::template push_front<head>;
    using type = typename std::conditional<std::is_placeholder<typename std::decay<arg_head>::type>::value,
                                           recursive, keep_head_recursive>::type;
  };

  template<class ParamList, class ArgList>
  using type = typename __helper<ParamList, ArgList>::type;
};

template<class ParamList, class ArgList>
struct remove_placeholders {
  using type = typename remove_placeholders_helper<ParamList::empty>::template type<ParamList, ArgList>;
};

template<class Ret, class ... Args>
Ret get_return_type(Ret(*)(Args...));

template<class Ret, class ... Args>
type_list<Args...> get_parameter_list(Ret(*)(Args...));

}

template<class FunTy>
struct function_traits {
  static_assert(std::is_function<FunTy>::value, "function expected.");
  using ptr_ty = typename std::decay<FunTy>::type;
  using return_type = decltype(get_return_type(ptr_ty()));
  using parameter_list = decltype(get_parameter_list(ptr_ty()));
};

template<class FunTy, class ArgList>
struct new_function_traits {
  using OriginalTraits = function_traits<FunTy>;
  using return_type = typename OriginalTraits::return_type;
  using parameter_list = typename remove_placeholders<typename OriginalTraits::parameter_list, ArgList>::type;
};

}
}



#endif
