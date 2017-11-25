// RUN: %clangxx %cxxflags %include_flags %s -o /dev/null

#include <easy/meta.h>
#include <type_traits>

using namespace easy;
using namespace easy::meta;

int foo(int, bool, float);
int baz(int, bool, decltype(std::placeholders::_1));

int main() {
  static_assert(std::is_same<
                            type_list<int, bool, float>::head,
                            int>::value,
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
  using baz_type = decltype(baz);
  using foo_traits = function_traits<foo_type>;
  using baz_traits = function_traits<baz_type>;

  static_assert(std::is_same<foo_traits::return_type, int>::value,
                "not same type");
  static_assert(std::is_same<
                            foo_traits::parameter_list,
                            type_list<int, bool, float>>::value,
                "not same type");
  static_assert(std::is_same<
                            typename meta::remove_placeholders<baz_traits::parameter_list>::type,
                            type_list<int, bool>>::value,
                "not same type");

  return 0;
}
