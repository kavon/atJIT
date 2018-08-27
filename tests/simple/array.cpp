// RUN: %atjitc   %s -o %t
// RUN: %t > %t.out

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

void writeTo1DArray(int sz, double A[]) {
  for (int i = 0; i < sz; ++i)
    A[i] = i;
}

void test1DArray() {
  const int DIM = 50;
  double Vec[DIM];

  auto const &Fn = easy::jit(writeTo1DArray, DIM, _1);
  Fn(Vec);
}



#define DIM 50
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

  test1DArray();
  test2DArray();

  return 0;
}
