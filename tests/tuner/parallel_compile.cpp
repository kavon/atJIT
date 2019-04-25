// RUN: %atjitc   %s -o %t
// RUN: %t

#include <tuner/driver.h>
#include <tuner/param.h>

#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cinttypes>

#include <atomic>

// just testing to see if we segfault or otherwise crash
// when stressing the parallel compilation pipeline as much
// as possible.

using namespace std::placeholders;
using namespace tuned_param;
using namespace easy::options;

std::atomic<int> dummy;

void spin(int val, std::atomic<int> &dummy) {
  for (int i = 0; i < val; i++)
    dummy = i;
}

int main(int argc, char** argv) {

  tuner::AutoTuner TunerKind = tuner::AT_Random;
  const int ITERS = 100;
  int minVal = 9999999;
  int maxVal = 99999999;
  int dflt = (maxVal - minVal) / 2;

  tuner::ATDriver AT;

  for (int i = 0; i < ITERS; i++) {
    auto const &OptimizedFun = AT.reoptimize(spin,
          IntRange(minVal, maxVal, dflt),
          dummy,
          tuner_kind(TunerKind),
          feedback_kind(tuner::FB_Total_IgnoreError),
          blocking(true)
          );

    OptimizedFun();
  }

  return 0;
}
