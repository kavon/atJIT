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

// indexing into flattened, dense 2D matrix
#define IDX(Mat, ColWidth, Row, Col) \
    Mat[(Row) * (ColWidth) + (Col)]

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
        IDX(C, N, m, n) += IDX(A, K, m, k) * IDX(B, N, k, n);
}


/////////////////////////////////////////////////////////////////


/////////////////////
// benchmark drivers

void verifyInit(int M, int N, int K, double *A, double *B) {
  const int ROW = 1;
  const int COL = 4;

  // clear a row of A
  for (int i = 0; i < M; i++)
    IDX(A, K, ROW, i) = 0;

  // set two elms of that row
  IDX(A, K, ROW, 2) = 2;
  IDX(A, K, ROW, 3) = 3;


  // clear a column of B
  for (int i = 0; i < N; i++)
    IDX(B, N, i, COL) = 0;

  // set two elms of that col
  IDX(B, N, 2, COL) = 5;
  IDX(B, N, 3, COL) = 7;
}

bool verifyCheck(int N, double *C) {
  // C[1][4] = A[1][2] * B[2][4] + A[1][3] * B[3][4]
  //         =    2    *    5    +    3    *    7
  //         =         10        +        21
  //         =                  31
  return IDX(C, N, 1, 4) == 31;
}




static void AOT_matmul(benchmark::State& state) {
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
  verifyInit(M, N, K, A.data(), B.data());

  // clear C
  std::fill(C.begin(), C.end(), 0);


  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      matmul(M, N, K, C.data(), A.data(), B.data());

      state.PauseTiming();

      assert(verifyCheck(N, C.data()));
      std::fill(C.begin(), C.end(), 0);

      benchmark::ClobberMemory();
      state.ResumeTiming();
    }
  }
}



static void JIT_matmul(benchmark::State& state) {
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

  // init with some non-zero junk
  std::iota(A.begin(), A.end(), 2);
  std::iota(B.begin(), B.end(), 5);

  // init part of the mats with known vals
  verifyInit(M, N, K, A.data(), B.data());

  // clear C
  std::fill(C.begin(), C.end(), 0);


  for (auto _ : state) {
    tuner::ATDriver AT;
    auto Tuner = easy::options::tuner_kind(TK);

    for (int i = 0; i < ITERS; i++) {
      auto const& my_matmul = AT.reoptimize(matmul,
        M, N, K, _1, _2, _3,
        Tuner
      );

      my_matmul(C.data(), A.data(), B.data());


      state.PauseTiming();

      assert(verifyCheck(N, C.data()));
      std::fill(C.begin(), C.end(), 0);

      benchmark::ClobberMemory();
      state.ResumeTiming();
    }
  }
}


static void TUNED_matmul(benchmark::State& state) {
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

  // init with some non-zero junk
  std::iota(A.begin(), A.end(), 2);
  std::iota(B.begin(), B.end(), 5);

  // init part of the mats with known vals
  verifyInit(M, N, K, A.data(), B.data());

  // clear C
  std::fill(C.begin(), C.end(), 0);


  for (auto _ : state) {
    tuner::ATDriver AT;
    auto Tuner = easy::options::tuner_kind(TK);

    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      auto const& my_matmul = AT.reoptimize(matmul,
        M, N, K, _1, _2, _3,
        Tuner
      );
      state.ResumeTiming();

      my_matmul(C.data(), A.data(), B.data());


      state.PauseTiming();

      assert(verifyCheck(N, C.data()));
      std::fill(C.begin(), C.end(), 0);

      benchmark::ClobberMemory();
      state.ResumeTiming();
    }
    // NOTE: we don't want to time the driver's destructor
    state.PauseTiming();
  }
}

/////////////////////////////
// benchmark registration

namespace mm {
  static constexpr int DIM_MIN = 200;
  static constexpr int DIM_MAX = 200;

  static constexpr int ITER_MIN = 50;
  static constexpr int ITER_MAX = 400;

#define MAIN_LOOP \
  for (int i = ITER_MIN; i <= ITER_MAX; i *= 2) \
    for (int sz = DIM_MIN; sz <= DIM_MAX; sz *= 2)

  void ArgGen(benchmark::internal::Benchmark* b) {
    MAIN_LOOP
      b->Args({sz, i});
  }

  void TunedArgGen(benchmark::internal::Benchmark* b) {
    for (tuner::AutoTuner TK : tuner::AllTuners)
      MAIN_LOOP
        b->Args({sz, i, TK});
  }

  #undef MAIN_LOOP
}

BENCHMARK(AOT_matmul)
  ->Unit(benchmark::kMillisecond)
  ->Apply(mm::ArgGen)
  ->UseRealTime();

BENCHMARK(TUNED_matmul)
  ->Unit(benchmark::kMillisecond)
  ->Apply(mm::TunedArgGen)
  ->UseRealTime();

BENCHMARK(JIT_matmul)
  ->Unit(benchmark::kMillisecond)
  ->Apply(mm::TunedArgGen)
  ->UseRealTime();

#undef IDX

#endif // BENCH_MATMUL
