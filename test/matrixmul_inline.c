// RUN: clang -I%includepath -S -emit-llvm %s -o %t.orig.ll
// RUN: opt -S %load_static_lib %t.orig.ll -o %t.jit.ll
// RUN: clang %t.jit.ll -O3 %ldflags -o %t.jit.exe
// RUN: %t.jit.exe

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "easyjit.h"

#define N 400

unsigned size = N;

typedef struct {
  double** rows;
  unsigned size;
} matrix;

matrix create_matrix(unsigned new_size, double val) {
  int i, j, k;
  matrix _new;
  _new.size = new_size;
  _new.rows = (double**)malloc(sizeof(double*) * new_size);

  for (i = 0; i < new_size; ++i) {
    _new.rows[i] = (double*)malloc(sizeof(double) * new_size);

    for (j = 0; j < new_size; ++j) {
      _new.rows[i][j] = val;
    }
  }
  return _new;
}

void delete_matrix(matrix* a) {
  int i;

  for (i = 0; i < a->size; ++i) {
    free(a->rows[i]);
  }
  free(a->rows);
  a->size = 0;
  a->rows = 0x0;
}

void kernel(matrix a, matrix b, matrix* out, long size) {
  easy_jit_enabled(3, size); // function marked for extraction

  int i, j, k;
  long n = size;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      out->rows[i][j] = 0.0;
      for (k = 0; k < n; ++k) {
        out->rows[i][j] += a.rows[i][k] * b.rows[k][j];
      }
    }
  }
}

int verify_kernel(matrix a, matrix b, matrix* out) {
  int i, j, k;
  unsigned n = a.size;
  int res = 0;

  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      double verif = 0.0;
      for (k = 0; k < n; ++k) {
        verif += a.rows[i][k] * b.rows[k][j];
      }

      if(fabs(out->rows[i][j] - verif) > 0.01) {
        printf("On element i=%d j=%d    expected=%f got=%f.\n", i, j, verif, out->rows[i][j]);
        res=1;
      }
    }
  }
  return res;
}

int main(int argc, char** argv) {
  int ret = 0;
  matrix min_a = create_matrix(size, 3.14),
         min_b = create_matrix(size, 3.14),
         mout = create_matrix(size, 0.0);

  kernel(min_a, min_b, &mout, size);
#ifndef NO_VERIF
  if(argc == 1)
	  ret = verify_kernel(min_a, min_b, &mout);
#endif

  printf("completed %lf.\n", mout.rows[0][0]);
  delete_matrix(&min_a);
  delete_matrix(&min_b);
  delete_matrix(&mout);
  return ret;
}
