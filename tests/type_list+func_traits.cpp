// RUN: %clangxx %cxxflags %include_flags %s -o /dev/null

#include <easy/meta.h>
#include <type_traits>

using namespace easy;
using namespace easy::meta;

int foo(int, bool, float);

int main() {
  static_assert(std::is_same<
                            type_list<int, bool, float>::head,
                            int>::value,
                "not same type");

  static_assert(std::is_same<
                            type_list<int, bool, float>::at<0>,
                            int>::value,
                "not same type");
  static_assert(std::is_same<
                            type_list<int, bool, float>::at<1>,
                            bool>::value,
                "not same type");
  static_assert(std::is_same<
                            type_list<int, bool, float>::at<2>,
                            float>::value,
                "not same type");
  static_assert(type_list<int, bool, float>::size == 3,
                "not correct size");
  static_assert(!type_list<int, bool, float>::empty,
                "detected as empty");
  static_assert(std::is_same<
                            type_list<int, bool, float>::tail::head,
                            bool>::value,
                "not same type");

  using foo_type = decltype(foo);
  using foo_traits = function_traits<foo_type>;

  static_assert(std::is_same<foo_traits::return_type, int>::value,
                "not same type");
  static_assert(std::is_same<
                            foo_traits::parameter_list,
                            type_list<int, bool, float>>::value,
                "not same type");

  static_assert(std::is_same<
                            typename meta::init_list<3,void>::type,
                            type_list<void, void, void>>::value,
                "not same type");
  static_assert(std::is_same<
                            typename meta::init_list<0,void>::type,
                            type_list<>>::value,
                "not same type");

  return 0;
}
