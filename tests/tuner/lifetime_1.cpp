// RUN: %atjitc   %s -o %t
// RUN: %valgrind --leak-check=summary %t 2> vgrind.out
// RUN: %FileCheck %s < vgrind.out

#include <tuner/driver.h>
#include <tuner/param.h>

#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cinttypes>

#include <atomic>

// test the destruction of the ATDriver for leaks

// CHECK:        LEAK SUMMARY:
// CHECK-NEXT:   definitely lost: 0 bytes in 0 blocks

using namespace std::placeholders;
using namespace tuned_param;

void doNothing(int val) {
  return;
}

int main(int argc, char** argv) {

  const int ITERS = 25;
  int minVal = 9999999;
  int maxVal = 99999999;
  int dflt = (maxVal - minVal) / 2;



  for (int i = 0; i < ITERS; i++) {
    tuner::ATDriver AT;
    auto const& Func1 = AT.reoptimize(doNothing, IntRange(minVal, maxVal, dflt));
    auto const& Func2 = AT.reoptimize(doNothing, IntRange(minVal, maxVal, dflt));

    Func1();
    Func2();
  }

  return 0;
}
