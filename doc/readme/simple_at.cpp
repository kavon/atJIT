// RUN: %atjitc   %s -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

// CHECK: dec(5) == 4.0
// CHECK: neg(3) == -3.0
// CHECK: 8 - 7 == 1.0


// INLINE FROM HERE #ALL#
#include <tuner/driver.h>
#include <cstdio>

// INLINE FROM HERE #USAGE#
using namespace std::placeholders;

float fsub(float a, float b) { return a-b; }
// TO HERE #USAGE#

int main () {
  tuner::ATDriver AT;
// INLINE FROM HERE #USAGE#
  // returns a function computing fsub(a, 1.0)
  easy::FunctionWrapper<float(float)> const& decrement =
    AT.reoptimize(fsub, _1, 1.0);

  // returns a function computing fsub(0.0, b)
  auto const& negate = AT.reoptimize(fsub, 0.0, _1);

  printf("dec(5) == %f\n", decrement(5));
  printf("neg(3) == %f\n", negate(3));
// TO HERE #USAGE#
// INLINE FROM HERE #TUNING#
  for (int i = 0; i < 100; ++i) {
    auto const& tunedSub7 =
        AT.reoptimize(fsub, _1, 7.0,
            easy::options::tuner_kind(tuner::AT_Random));

    printf("8 - 7 == %f\n", tunedSub7(8));
  }
// TO HERE #TUNING#
}
// TO HERE #ALL#
