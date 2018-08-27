// REQUIRES: pollyknobs
// RUN: rm -f %t.ll
// RUN: %atjitc   %s -o %t
// RUN: %t %t.ll
// RUN: %FileCheck --check-prefix=CHECK-IR %s < %t.ll

#include <tuner/driver.h>
#include <tuner/param.h>

#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cinttypes>

using namespace std::placeholders;
using namespace easy::options;

////////////

// make sure polly did something
// CHECK-IR: polly.loop_header

// make sure the tiling metadata was consumed too
// CHECK-IR-NOT: llvm.loop.tile


void pragma_id_tile(float *Mat, const int SZ) {
  for (int i = 0; i < SZ; i += 1)
    for (int j = 0; j < SZ; j += 1)
      Mat[(i * SZ) + j] = i + j;
}

#define DIM 2000

////////////


int main(int argc, char** argv) {

  tuner::ATDriver AT;
  const int ITERS = 100;
  std::vector<float> Mat(DIM * DIM);

  for(int i = 0; i < ITERS; i++) {
    auto const &OptimizedFun = AT.reoptimize(pragma_id_tile,
          _1, _2,
          tuner_kind(tuner::AT_Random)
          , dump_ir(argv[argc-1])
          , blocking(true)
        );

    OptimizedFun(Mat.data(), DIM);
  }

  return 0;
}
