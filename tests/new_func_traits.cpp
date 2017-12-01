// RUN: %clangxx %cxxflags %include_flags %s -o /dev/null

#include <easy/meta.h>
#include <type_traits>
#include <iostream>

using namespace easy;
using namespace easy::meta;
using namespace std::placeholders;

int foo(int, bool, float, int);

int main() {

  using foo_type = decltype(foo);
  using new_foo_traits_a = new_function_traits<foo_type, meta::type_list<int, bool, float, int>>;
  using new_foo_traits_b = new_function_traits<foo_type, meta::type_list<int, bool, float, long>>;
  using new_foo_traits_c = new_function_traits<foo_type, meta::type_list<int, bool, double, long>>;
  using new_foo_traits_d = new_function_traits<foo_type, meta::type_list<decltype (_1), bool, float, decltype (_4)>>;
  using new_foo_traits_e = new_function_traits<foo_type, meta::type_list<decltype (_1), bool, float, decltype (_1)>>;
  using new_foo_traits_f = new_function_traits<foo_type, meta::type_list<decltype (_4), bool, float, decltype (_4)>>;

  // full specialization
  static_assert(new_foo_traits_a::parameter_list::empty, "fail A not empty");
  static_assert(new_foo_traits_b::parameter_list::empty, "fail B not empty");
  static_assert(new_foo_traits_c::parameter_list::empty, "fail C not empty");

  // two int parameters
  static_assert(new_foo_traits_d::parameter_list::size == 2, "fail D size");
  static_assert(std::is_same<
                  typename new_foo_traits_d::parameter_list,
                  meta::type_list<bool, float>
                >::value, "fail D types");

  // one int parameter
  static_assert(new_foo_traits_e::parameter_list::size == 1, "fail E size");
  static_assert(new_foo_traits_f::parameter_list::size == 1, "fail F size");

  static_assert(std::is_same<
                  typename new_foo_traits_e::parameter_list,
                  meta::type_list<int>
                >::value, "fail E types");
  static_assert(std::is_same<
                typename new_foo_traits_f::parameter_list,
                meta::type_list<int>
              >::value, "fail F types");

  return 0;
}
