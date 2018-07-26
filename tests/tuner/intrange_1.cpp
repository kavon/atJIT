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

void show(int i, int k) {
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  printf("intrange test recieved (%i, %i)\n", i, k);
}

// (1) make sure the dynamic arg is still making it through

// CHECK: intrange test recieved ({{[34]}}, 55)

// (2) make sure the range is inclusive

// RUN: grep "intrange test recieved (3" < %t.out
// RUN: grep "intrange test recieved (4" < %t.out

int main(int argc, char** argv) {

  tuner::AutoTuner TunerKind = tuner::AT_Random;
  const int ITERS = 500;

  tuner::ATDriver AT;

  for (int i = 0; i < ITERS; i++) {
    auto const &OptimizedFun = AT.reoptimize(show,
          IntRange(3, 4), _1,
          easy::options::tuner_kind(TunerKind));

    OptimizedFun(i);
  }

  return 0;
}
