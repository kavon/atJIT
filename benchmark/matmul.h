#ifndef BENCH_MATMUL
#define BENCH_MATMUL

///////////////////////////
// benchmark code

// "restrict" is not a C++ keyword/type qualifier,
// but compilers recognize the hint in c++

#if defined(__GNUC__) || defined(__clang__)
  #define NOALIAS __restrict__
#else
  #error "add your compiler here"
#endif

// computes  C = AB  where
// C is an MxN dense matrix
// A is an MxK dense matrix
// B is an KxN dense matrix
// and all matrices are non-overlapping in memory.
__attribute__((noinline))
void matmul(int M, int N, int K, double * NOALIAS C, double * NOALIAS A, double * NOALIAS B) {
  for (int m = 0; m < M; m += 1)
    for (int n = 0; n < N; n += 1)
      for (int k = 0; k < K; k += 1)
        C[m * M + n] += A[m * M + k] * B[k * K + n];
}


/////////////////////////////////////////////////////////////////


/////////////////////
// benchmark driver

static void BM_matmul(benchmark::State& state) {
  // TODO: maybe we should work with non-square cases?
  const int DIM = state.range(0);
  const int ITERS = state.range(1);

  const int M = DIM;
  const int N = DIM;
  const int K = DIM;

  // alloc
  std::vector<double> A(M*K);
  std::vector<double> B(K*N);
  std::vector<double> C(M*N);

  // init
  std::iota(A.begin(), A.end(), 0);
  std::iota(B.begin(), B.end(), 0);


  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      std::fill(C.begin(), C.end(), 0.0);

      benchmark::ClobberMemory();
      state.ResumeTiming();

      matmul(DIM, DIM, DIM, C.data(), A.data(), B.data());
    }
  }
}

namespace mm {
  static constexpr int DIM_MIN = 200;
  static constexpr int DIM_MAX = 200;

  static constexpr int ITER_MIN = 32;
  static constexpr int ITER_MAX = 128;
}

/////////////////////////////
// benchmark registration

BENCHMARK(BM_matmul)
  ->Unit(benchmark::kMillisecond)
  ->RangeMultiplier(2)
  ->Ranges({{mm::DIM_MIN, mm::DIM_MAX}, {mm::ITER_MIN, mm::ITER_MAX}})
  ->UseRealTime();

#endif // BENCH_MATMUL
