// RUN: %atjitc   %s -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <tuner/driver.h>
#include <tuner/param.h>

#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cinttypes>

#include <chrono>
#include <thread>

using namespace std::placeholders;
using namespace tuned_param;
using namespace easy::options;

void show(int i, int k) {
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  printf("intrange test recieved (%i, %i)\n", i, k);
}

// (1) make sure the dynamic arg is still making it through

// CHECK: intrange test recieved ({{-?[0-9]}}, 1)
// CHECK: intrange test recieved ({{-?[0-9]}}, 3)

// (2) make sure the value changes within the range

// RUN: grep "intrange test recieved (9" < %t.out
// RUN: grep -E "intrange test recieved \(-?[0-8]" < %t.out

// (3) TODO: some sort of test that ensures the range is
// inclusive. Random tuning of even a [1,2] range is not stable
// enough for CI.

int main(int argc, char** argv) {

  tuner::AutoTuner TunerKind = tuner::AT_Random;
  const int ITERS = 5;

  tuner::ATDriver AT;

  for (int i = 0; i < ITERS; i++) {
    auto const &OptimizedFun = AT.reoptimize(show,
          IntRange(-8, 9, 9), _1,
          tuner_kind(TunerKind), pct_err(-1), blocking(true));

    OptimizedFun(i);
  }

  return 0;
}
