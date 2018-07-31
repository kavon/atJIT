// RUN: %atjitc   %s -o %t
// RUN: %t > %t.out
// RUN: %FileCheck %s < %t.out

#include <tuner/driver.h>

#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cinttypes>

using namespace std::placeholders;

///////////////////
// utilities for square matrices

template<typename T>
T** calloc_mat(const int DIM) {
  T** rows = (T**) malloc(DIM * sizeof(T*));
  for(int i = 0; i < DIM; i++) {
    rows[i] = (T*) calloc(DIM, sizeof(T));
  }
  return rows;
}

void free_mat(const int DIM, void** mat) {
  for (int i = 0; i < DIM; i++)
    free(mat[i]);
  free(mat);
}

template<typename T>
bool equal_mat(const int DIM, T** a, T** b) {
  for (int i = 0; i < DIM; i++)
    for (int k = 0; k < DIM; k++)
      if (a[i][k] != b[i][k])
        return false;

  return true;
}

////////////

// multiply square matrices
template <typename T>
T** MatMul(const int DIM, T** aMatrix, T** bMatrix) {
  T** product = calloc_mat<T>(DIM);
  for (int row = 0; row < DIM; row++) {
    for (int col = 0; col < DIM; col++) {
      // Multiply the row of A by the column of B to get the row, column of product.
      for (int inner = 0; inner < DIM; inner++) {
        product[row][col] += aMatrix[row][inner] * bMatrix[inner][col];
      }
    }
  }
  return product;
}

using ElmTy = int16_t;

void testWith(tuner::AutoTuner TunerKind, const int ITERS) {
  tuner::ATDriver AT;

  int DIM = 100;

  // initialize matrices
  ElmTy** aMatrix = calloc_mat<ElmTy>(DIM);
  ElmTy** bMatrix = calloc_mat<ElmTy>(DIM);

  for (int i = 0; i < DIM; i++) {
    for (int k = 0; k < DIM; k++) {
      if (i == k)
        aMatrix[i][k] = 1;
      else
        aMatrix[i][k] = 0;

      bMatrix[i][k] = (ElmTy) (i+k);
    }
  }

  for (int i = 0; i < ITERS; i++) {
    auto const &OptimizedFun = AT.reoptimize(MatMul<ElmTy>, DIM, _1, _2,
          easy::options::tuner_kind(TunerKind));

    ElmTy** ans = OptimizedFun(aMatrix, bMatrix);

    if (!equal_mat<ElmTy>(DIM, ans, bMatrix)) {
      printf("ERROR!! unexpected matrix multiply result.\n");
      std::exit(1);
    }

    free(ans);
  }

  free(aMatrix);
  free(bMatrix);

}

int main(int argc, char** argv) {

  // CHECK: [sq_matmul] start!
  printf("[sq_matmul] start!\n");

  testWith(tuner::AT_None, 5);
  // CHECK: [sq_matmul] noop tuner works
  printf("[sq_matmul] noop tuner works\n");

  testWith(tuner::AT_Random, 150);
  // CHECK: [sq_matmul] random tuner works
  printf("[sq_matmul] random tuner works\n");

  testWith(tuner::AT_Bayes, 150);
  // CHECK: [sq_matmul] bayes tuner works
  printf("[sq_matmul] bayes tuner works\n");

  testWith(tuner::AT_Anneal, 150);
  // CHECK: [sq_matmul] annealing tuner works
  printf("[sq_matmul] annealing tuner works\n");

  return 0;
}
