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

// CHECK: intrange test recieved ({{[34]}}, 1)
// CHECK: intrange test recieved ({{[34]}}, 7)

// (2) make sure the range is inclusive

// RUN: grep "intrange test recieved (3" < %t.out
// RUN: grep "intrange test recieved (4" < %t.out

int main(int argc, char** argv) {

  tuner::AutoTuner TunerKind = tuner::AT_Random;
  const int ITERS = 20; // 1/(2^9) chance this is not high enough to see both

  tuner::ATDriver AT;

  for (int i = 0; i < ITERS; i++) {
    auto const &OptimizedFun = AT.reoptimize(show,
          IntRange(3, 4), _1,
          tuner_kind(TunerKind), pct_err(50.0));

    OptimizedFun(i);
  }

  return 0;
}
