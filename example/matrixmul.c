#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "../include/easyjit.h"

#define N 400

unsigned size = N;

typedef struct {
  double** rows;
  unsigned size;
} matrix;

void check(long l) {
	printf("\t%ld\n", l);
}

void check_addr(void *p) {
	printf("\t\t%p\n", p);
}

matrix create_matrix(unsigned new_size, double val) {
  int i, j, k;
  matrix _new;
  _new.size = new_size;
  _new.rows = malloc(sizeof(double*) * new_size);

  for (i = 0; i < new_size; ++i) {
    _new.rows[i] = malloc(sizeof(double) * new_size);

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

void kernel(matrix a, matrix b, matrix* out) {
  easy_jit_enabled();

  int i, j, k;
  int n = a.size;

    for (i = 0; i < n; ++i) {
      for (j = 0; j < n; ++j) {
        out->rows[i][j] = 0.0;
        for (k = 0; k < n; ++k) {
	  out->rows[i][j] += a.rows[i][k] * b.rows[k][j];
        }
      }
  }
}

void verify_kernel(matrix a, matrix b, matrix* out) {
  int i, j, k;
  unsigned n = a.size;

    for (i = 0; i < n; ++i) {
      for (j = 0; j < n; ++j) {
	double verif = 0.0;
        for (k = 0; k < n; ++k) {
          verif += a.rows[i][k] * b.rows[k][j];
        }

        if(fabs(out->rows[i][j] - verif) > 0.01) {
		printf("On element i=%d j=%d    expected=%f got=%f.\n", i, j, verif, out->rows[i][j]);
	}
      }
    }
}

int main(int argc, char** argv) {
  matrix min_a = create_matrix(size, 3.14), min_b = create_matrix(size, 3.14), mout = create_matrix(size, 0.0);

  kernel(min_a, min_b, &mout);
#ifndef NO_VERIF
  if(argc == 1)
	  verify_kernel(min_a, min_b, &mout);
#endif

  printf("completed %lf.\n", mout.rows[0][0]);
  delete_matrix(&min_a);
  delete_matrix(&min_b);
  delete_matrix(&mout);
  return 0;
}
