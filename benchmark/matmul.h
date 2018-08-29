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
        C[m * N + n] += A[m * K + k] * B[k * N + n];
}


/////////////////////////////////////////////////////////////////


/////////////////////
// benchmark drivers

void verifyInit(int N, int K, double *A, double *B) {
  A[1 * K + 2] = 2;
  A[1 * K + 3] = 3;

  B[2 * N + 4] = 5;
  B[3 * N + 4] = 7;
}

bool verifyCheck(int N, double *C) {
  // C[1][4] = A[1][2] * B[2][4] + A[1][3] * B[3][4]
  //         =    2    *    5    +    3    *    7
  //         =         10        +        21
  //         =                  31

  double val = C[1 * N + 4];
  std::cerr << val << std::endl;
  return val == 31;
}

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

  // init with some non-zero junk
  std::iota(A.begin(), A.end(), 2);
  std::iota(B.begin(), B.end(), 5);

  // init part of the mats with known vals
  verifyInit(N, K, A.data(), B.data());

  // clear C
  std::fill(C.begin(), C.end(), 0.0);


  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      matmul(M, N, K, C.data(), A.data(), B.data());

      state.PauseTiming();

      assert(verifyCheck(N, C.data()));
      std::fill(C.begin(), C.end(), 0.0);

      benchmark::ClobberMemory();
      state.ResumeTiming();
    }
  }
}



static void BM_matmul_tuned(benchmark::State& state) {
  // TODO: maybe we should work with non-square cases?
  const int DIM = state.range(0);
  const int ITERS = state.range(1);
  tuner::AutoTuner TK = static_cast<tuner::AutoTuner>(state.range(2));

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
    tuner::ATDriver AT;
    auto Tuner = easy::options::tuner_kind(TK);

    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      std::fill(C.begin(), C.end(), 0.0);

      benchmark::ClobberMemory();
      state.ResumeTiming();

      auto const& my_matmul = AT.reoptimize(matmul,
        M, N, K, _1, _2, _3,
        Tuner);

      my_matmul(C.data(), A.data(), B.data());
    }
  }
}

/////////////////////////////
// benchmark registration

namespace mm {
  static constexpr int DIM_MIN = 200;
  static constexpr int DIM_MAX = 200;

  static constexpr int ITER_MIN = 32;
  static constexpr int ITER_MAX = 128;

  static void ArgGen(benchmark::internal::Benchmark* b) {
    for (tuner::AutoTuner TK : tuner::AllTuners)
      for (int i = ITER_MIN; i <= ITER_MAX; i *= 2)
        for (int sz = DIM_MIN; sz <= DIM_MAX; sz *= 2)
          b->Args({sz, i, TK});
  }
}

BENCHMARK(BM_matmul)
  ->Unit(benchmark::kMillisecond)
  ->RangeMultiplier(2)
  ->Ranges({{mm::DIM_MIN, mm::DIM_MAX}, {mm::ITER_MIN, mm::ITER_MAX}})
  ->UseRealTime();

  BENCHMARK(BM_matmul_tuned)
    ->Unit(benchmark::kMillisecond)
    ->Apply(mm::ArgGen)
    ->UseRealTime();

#endif // BENCH_MATMUL
