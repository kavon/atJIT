#ifndef META
#define META

namespace easy {
namespace meta {

template<class T> 
struct type_list { 
  static constexpr size_t size = 0;
};

template<class Head, class ... Tail> 
struct type_list {
  typedef Head head;
  typedef struct type_list<Tail...> tail;
  static constexpr size_t size = 1+tail::size;
};

namespace {

template<class Ret, class ... Args>
Ret get_return_type(Ret(Args...));

template<class Ret, class ... Args>
type_list<Args...> get_parameter_list(Ret(Args...));

}

template<class FunTy>
struct function_traits {
  typedef return_type = decltype(get_return_type(FunTy()));
  typedef parameter_list = decltype(get_parameter_list(FunTy()));
};

}
}



#endif
