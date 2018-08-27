// RUN: %not %atjitc   %s -o %t

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

#define DIM 49
void writeTo2DArray(double M[DIM][DIM]) {
  for (int i = 0; i < DIM; i += 1)
    for (int j = 0; j < DIM; j += 1)
      M[i][j] = i + j;
}
#undef DIM


void test2DArray() {
  const int DIM = 50;
  double Mat[DIM][DIM];

  auto const &Fn = easy::jit(writeTo2DArray, _1);
  Fn(Mat);
}

////////////


int main(int argc, char** argv) {

  test2DArray();

  return 0;
}
