#ifndef BENCH_QSORT
#define BENCH_QSORT

/* To sort array elements */

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

bool isSorted(int v[], int lo, int hi, int (*cmp)(int, int)) {
  if ((hi - lo + 1) < 2)
    return true;

  for (int i = lo+1; i <= hi; i++) {
    if (cmp(v[i-1], v[i]) > 0)
      return false;
  }
  return true;
}

/* swap: interchange v[i] and v[j] */
void swap(int v[], int i, int j)
{
    int temp;
    temp = v[i];
    v[i] = v[j];
    v[j] = temp;
}

void isort(int v[], int lo, int hi, int (*cmp)(int, int)) {
  int i = lo+1;
  while (i <= hi) {
    int j = i;
    while (j > lo && cmp(v[j-1], v[j]) > 0) {
      swap(v, j-1, j);
      j--;
    }
    i++;
  }
}

// https://en.wikipedia.org/wiki/Quicksort
// with modifications to support a cutoff to switch to insertion sort
void __attribute__((noinline)) Qsort(int v[], int left, int right, int (*cmp)(int, int), int cutOff, int dummy)
{
    int sz = right - left + 1;

    if (sz < 2)  // do nothing if array contains < 2 elems
      return;

    if (sz <= cutOff) {
      // have insertion sort handle the slice
      isort(v, left, right, cmp);
      return;
    }

    int pivot = v[right];
    int i = left;
    for (int j = left; j < right; j++)  // partition
      if (cmp(v[j], pivot) < 0) {
        swap(v, i, j);
        i++;
      }

    swap(v, i, right);                // emplace pivot

    Qsort(v, left, i-1, cmp, cutOff, dummy);
    Qsort(v, i+1, right, cmp, cutOff, dummy);
}


/////////////////////////////////////////////////////////////////

#define ISORT_MAX_CUTOFF 512
#define ISORT_MIN_CUTOFF 4
#define ISORT_IDEAL_CUTOFF 32

#define QSORT_MIN 2048
#define QSORT_MAX 2048
#define ITER_MIN 128
#define ITER_MAX 512

using namespace tuned_param;

static void BM_qsort_jit_cache(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  const int ITERS = state.range(1);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  easy::Cache<> cache;

  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      assert( isSorted(vec.data(), 0, vec.size()-1, int_cmp) );
      std::random_shuffle(vec.begin(), vec.end());
      benchmark::ClobberMemory();
      state.ResumeTiming();

      auto const& my_qsort = cache.jit(Qsort, _1, _2, _3, int_cmp, ISORT_IDEAL_CUTOFF, n);

      my_qsort(vec.data(), 0, vec.size()-1);
    }
  }
}
BENCHMARK(BM_qsort_jit_cache)
  ->Unit(benchmark::kMillisecond)
  ->RangeMultiplier(2)
  ->Ranges({{QSORT_MIN, QSORT_MAX}, {ITER_MIN, ITER_MAX}});


static void BM_qsort_tuned_bayes(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  const int ITERS = state.range(1);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  tuner::ATDriver AT;
  auto Tuner = easy::options::tuner_kind(tuner::AT_Bayes);
  auto Range = IntRange(ISORT_MIN_CUTOFF, ISORT_MAX_CUTOFF, ISORT_IDEAL_CUTOFF);

  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      assert( isSorted(vec.data(), 0, vec.size()-1, int_cmp) );
      std::random_shuffle(vec.begin(), vec.end());
      benchmark::ClobberMemory();
      state.ResumeTiming();

      auto const& my_qsort = AT.reoptimize(Qsort, _1, _2, _3, int_cmp,
         Range, n, Tuner);

      my_qsort(vec.data(), 0, vec.size()-1);
    }
  }

}
BENCHMARK(BM_qsort_tuned_bayes)
  ->Unit(benchmark::kMillisecond)
  ->RangeMultiplier(2)
  ->Ranges({{QSORT_MIN, QSORT_MAX}, {ITER_MIN, ITER_MAX}});



static void BM_qsort_tuned_anneal(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  const int ITERS = state.range(1);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  tuner::ATDriver AT;
  auto Tuner = easy::options::tuner_kind(tuner::AT_Anneal);
  auto Range = IntRange(ISORT_MIN_CUTOFF, ISORT_MAX_CUTOFF, ISORT_IDEAL_CUTOFF);

  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      assert( isSorted(vec.data(), 0, vec.size()-1, int_cmp) );
      std::random_shuffle(vec.begin(), vec.end());
      benchmark::ClobberMemory();
      state.ResumeTiming();

      auto const& my_qsort = AT.reoptimize(Qsort, _1, _2, _3, int_cmp,
         Range, n, Tuner);

      my_qsort(vec.data(), 0, vec.size()-1);
    }
  }

}
BENCHMARK(BM_qsort_tuned_anneal)
  ->Unit(benchmark::kMillisecond)
  ->RangeMultiplier(2)
  ->Ranges({{QSORT_MIN, QSORT_MAX}, {ITER_MIN, ITER_MAX}});


  static void BM_qsort_tuned_random(benchmark::State& state) {
    using namespace std::placeholders;
    int n = state.range(0);
    const int ITERS = state.range(1);
    std::vector<int> vec(n);
    std::iota(vec.begin(), vec.end(), 0);

    tuner::ATDriver AT;
    auto Tuner = easy::options::tuner_kind(tuner::AT_Random);
    auto Range = IntRange(ISORT_MIN_CUTOFF, ISORT_MAX_CUTOFF, ISORT_IDEAL_CUTOFF);

    for (auto _ : state) {
      for (int i = 0; i < ITERS; i++) {
        state.PauseTiming();
        assert( isSorted(vec.data(), 0, vec.size()-1, int_cmp) );
        std::random_shuffle(vec.begin(), vec.end());
        benchmark::ClobberMemory();
        state.ResumeTiming();

        auto const& my_qsort = AT.reoptimize(Qsort, _1, _2, _3, int_cmp,
           Range, n, Tuner);

        my_qsort(vec.data(), 0, vec.size()-1);
      }
    }

  }
  BENCHMARK(BM_qsort_tuned_random)
    ->Unit(benchmark::kMillisecond)
    ->RangeMultiplier(2)
    ->Ranges({{QSORT_MIN, QSORT_MAX}, {ITER_MIN, ITER_MAX}});


static void BM_qsort(benchmark::State& state) {
  int n = state.range(0);
  const int ITERS = state.range(1);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      assert( isSorted(vec.data(), 0, vec.size()-1, int_cmp) );
      std::random_shuffle(vec.begin(), vec.end());
      benchmark::ClobberMemory();
      state.ResumeTiming();

      Qsort(vec.data(), 0, vec.size()-1, int_cmp, ISORT_IDEAL_CUTOFF, n);
    }
  }
}
BENCHMARK(BM_qsort)
  ->Unit(benchmark::kMillisecond)
  ->RangeMultiplier(2)
  ->Ranges({{QSORT_MIN, QSORT_MAX}, {ITER_MIN, ITER_MAX}});


#endif // BENCH_QSORT
