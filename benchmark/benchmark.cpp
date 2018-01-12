
#include <benchmark/benchmark.h>
#include <easy/jit.h>

void kernel(int n, int m, int * image, int * mask, int* out) __attribute__((noinline)) {
  for(int i = 0; i < n - m; ++i)
    for(int j = 0; j < n - m; ++j)
      for(int k = 0; k < m; ++k)
        for(int l = 0; l < m; ++l)
          out[i * (n-m+1) + j] += image[(i+k) * n + j+l] * mask[k *m + l];
}

/* To sort array elemets */

int int_cmp(int a, int b)
{
  if (a > b)
    return 1;
  else
  {
    if (a == b)
      return 0;
    else
      return -1;
  }
}

// https://github.com/ctasims/The-C-Programming-Language--Kernighan-and-Ritchie/blob/master/ch04-functions-and-program-structure/qsort.c
void Qsort(int v[], int left, int right, int (*cmp)(int, int)) __attribute__((noinline))
{
    int i, last;
    void swap(int v[], int i, int j);

    if (left >= right)  // do nothing if array contains < 2 elems
        return;
    // move partition elem to v[0]
    swap(v, left, (left + right)/2);
    last = left;

    for (i = left+1; i <= right; i++)  // partition
        if (cmp(v[i], v[left]))
            swap(v, ++last, i);

    swap(v, left, last);                // restore partition elem
    Qsort(v, left, last-1, cmp);
    Qsort(v, last+1, right, cmp);
}

/* swap: interchange v[i] and v[j] */
void swap(int v[], int i, int j)
{
    int temp;
    temp = v[i];
    v[i] = v[j];
    v[j] = temp;
}


int image[11][11] = {
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
  {1,2,3,4,5,6,5,4,3,2,1},
};
int mask[3][3] = {{1,2,3},{0,0,0},{3,2,1}};

static void BM_convolve_jit(benchmark::State& state) {
  using namespace std::placeholders;
  int out[9][9] = {{0}};
  auto my_kernel = easy::jit(kernel, 11, 3, _1, _2, _3);
  for (auto _ : state) {
    my_kernel(&image[0][0], &mask[0][0], &out[0][0]);
    volatile int touch = out[2][2];
  }
}
// Register the function as a benchmark
BENCHMARK(BM_convolve_jit);

// Define another benchmark
static void BM_convolve(benchmark::State& state) {
  int out[9][9] = {{0}};
  for (auto _ : state) {
    kernel(11, 3, &image[0][0], &mask[0][0], &out[0][0]);
    volatile int touch = out[2][2];
  }
}
BENCHMARK(BM_convolve);

static void BM_qsort_jit(benchmark::State& state) {
  using namespace std::placeholders;
  auto my_qsort = easy::jit(Qsort, _1, _2, _3, int_cmp);
  for (auto _ : state) {
    int data[90] = {3,2,5,4,6,7,9,8, 1,78,};
    my_qsort(data, 0, 89);
    volatile int touch = data[0];
  }
}
// Register the function as a benchmark
BENCHMARK(BM_qsort_jit);

// Define another benchmark
static void BM_qsort(benchmark::State& state) {
  for (auto _ : state) {
    int data[90] = {3,2,5,4,6,7,9,8, 1,78,};
    Qsort(data, 0, 89, int_cmp);
    volatile int touch = data[0];
  }
}
BENCHMARK(BM_qsort);

static void BM_jit(benchmark::State& state) {
  using namespace std::placeholders;
  for (auto _ : state) {
    volatile auto my_qsort = easy::jit(Qsort, _1, _2, _3, int_cmp);
  }
}
BENCHMARK(BM_jit);

BENCHMARK_MAIN();
