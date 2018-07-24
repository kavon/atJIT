// RUN: %atjitc   %s -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

// CHECK: dec(5) == 4.0
// CHECK: neg(3) == -3.0
// CHECK: fsubJIT(3, 2) == 1.0
// CHECK: 8 - 7 == 1.0


// INLINE FROM HERE #ALL#
#include <tuner/driver.h>
#include <cstdio>

// INLINE FROM HERE #USAGE#
using namespace std::placeholders;

float fsub(float a, float b) { return a-b; }

int main () {
  tuner::ATDriver AT;

  // returns a function computing fsub(a, 1.0)
  easy::FunctionWrapper<float(float)> const& decrement =
      AT.reoptimize(fsub, _1, 1.0);

  // returns a function computing fsub(0.0, b)
  auto const& negate = AT.reoptimize(fsub, 0.0, _1);

  printf("dec(5) == %f\n", decrement(5));
  printf("neg(3) == %f\n", negate(3));
  // ...
// TO HERE #USAGE#

// INLINE FROM HERE #TUNERKIND#
using namespace easy::options;

// returns a function equivalent to fsub(a, b)
auto const& fsubJIT = AT.reoptimize(fsub, _1, _2,
                           tuner_kind(tuner::AT_Random));

printf("fsubJIT(3, 2) == %f\n", fsubJIT(3.0, 2.0));
// TO HERE #TUNERKIND#

// INLINE FROM HERE #TUNING#
  for (int i = 0; i < 100; ++i) {
    auto const& tunedSub7 =
        AT.reoptimize(fsub, _1, 7.0, tuner_kind(tuner::AT_Random));

    printf("8 - 7 == %f\n", tunedSub7(8));
  }
// TO HERE #TUNING#
}
// TO HERE #ALL#
