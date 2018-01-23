
#include <benchmark/benchmark.h>
#include <easy/cache.h>
#include <numeric>

void __attribute__((noinline)) kernel(int n, int m, int * image, int const * mask, int* out) {
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
void __attribute__((noinline)) Qsort(int v[], int left, int right, int (*cmp)(int, int)) 
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

static const int mask[3][3] = {{1,2,3},{0,0,0},{3,2,1}};

static void BM_convolve_jit(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  std::vector<int> image(n*n,0);
  std::vector<int> out((n-3)*(n-3),0);
  benchmark::ClobberMemory();

  auto my_kernel = easy::jit(kernel, n, 3, _1, &mask[0][0], _2);
  for (auto _ : state) {
    my_kernel(image.data(), out.data());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_convolve_jit)->RangeMultiplier(2)->Range(16,1024);

static void BM_convolve(benchmark::State& state) {
  int n = state.range(0);
  std::vector<int> image(n*n,0);
  std::vector<int> out((n-3)*(n-3),0);
  benchmark::ClobberMemory();

  for (auto _ : state) {
    kernel(n, 3, image.data(), &mask[0][0], out.data());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_convolve)->RangeMultiplier(2)->Range(16,1024);

static void BM_convolve_compile_jit(benchmark::State& state) {
  using namespace std::placeholders;
  for (auto _ : state) {
    auto my_kernel = easy::jit(kernel, 11, 3, _1, &mask[0][0], _2);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_convolve_compile_jit);

static void BM_convolve_cache_hit_jit(benchmark::State& state) {
  using namespace std::placeholders;
  static easy::Cache cache;
  cache.jit(kernel, 11, 3, _1, &mask[0][0], _2);
  benchmark::ClobberMemory();

  for (auto _ : state) {
    auto const &my_kernel = cache.jit(kernel, 11, 3, _1, &mask[0][0], _2);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_convolve_cache_hit_jit);

static void BM_qsort_jit(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);
  std::random_shuffle(vec.begin(), vec.end());
  benchmark::ClobberMemory();

  auto my_qsort = easy::jit(Qsort, _1, _2, _3, int_cmp);
  for (auto _ : state) {
    my_qsort(vec.data(), 0, vec.size()-1);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_qsort_jit)->RangeMultiplier(2)->Range(16,1024);

static void BM_qsort(benchmark::State& state) {
  int n = state.range(0);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);
  std::random_shuffle(vec.begin(), vec.end());
    benchmark::ClobberMemory();

  for (auto _ : state) {
    Qsort(vec.data(), 0, vec.size()-1, int_cmp);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_qsort)->RangeMultiplier(2)->Range(16,1024);

static void BM_qsort_compile_jit(benchmark::State& state) {
  using namespace std::placeholders;
  for (auto _ : state) {
    auto my_qsort = easy::jit(Qsort, _1, _2, _3, int_cmp);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_qsort_compile_jit);

static void BM_qsort_cache_hit_jit(benchmark::State& state) {
  using namespace std::placeholders;
  static easy::Cache cache;
  cache.jit(Qsort, _1, _2, _3, int_cmp);
  benchmark::ClobberMemory();
  for (auto _ : state) {
    auto const &my_qsort = cache.jit(Qsort, _1, _2, _3, int_cmp);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_qsort_cache_hit_jit);

BENCHMARK_MAIN();
