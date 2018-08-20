#ifndef BENCH_CONVOLVE
#define BENCH_CONVOLVE


void __attribute__((noinline)) kernel(int n, int m, int * image, int const * mask, int* out) {
  for(int i = 0; i < n - m; ++i)
    for(int j = 0; j < n - m; ++j)
      for(int k = 0; k < m; ++k)
        for(int l = 0; l < m; ++l)
          out[i * (n-m+1) + j] += image[(i+k) * n + j+l] * mask[k *m + l];
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
// NOTE: this test was disabled due to it crashing sometimes. See
// here for more info:  https://github.com/kavon/atJIT/issues/2
//
// BENCHMARK(BM_convolve_jit)->RangeMultiplier(2)->Range(16,1024);

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
// NOTE: this test sometimes segfaults above 512 too!
// https://travis-ci.org/kavon/atJIT/builds/416869958
// BENCHMARK(BM_convolve)->RangeMultiplier(2)->Range(16,1024);

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
  static easy::Cache<> cache;
  cache.jit(kernel, 11, 3, _1, &mask[0][0], _2);
  benchmark::ClobberMemory();

  for (auto _ : state) {
    auto const &my_kernel = cache.jit(kernel, 11, 3, _1, &mask[0][0], _2);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_convolve_cache_hit_jit);

#endif
