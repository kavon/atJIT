#ifndef BENCH_QSORT
#define BENCH_QSORT

///////////////////////////
// benchmark code

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
void __attribute__((noinline)) Qsort(int v[], int left, int right, int (*cmp)(int, int), int cutOff)
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

    Qsort(v, left, i-1, cmp, cutOff);
    Qsort(v, i+1, right, cmp, cutOff);
}


/////////////////////////////////////////////////////////////////


/////////////////////
// benchmark driver

#define ISORT_MAX_CUTOFF 512
#define ISORT_MIN_CUTOFF 4
#define ISORT_IDEAL_CUTOFF 32

static void BM_qsort_tuned(benchmark::State& state) {
  const int SZ = state.range(0);
  const int ITERS = state.range(1);
  tuner::AutoTuner TK = static_cast<tuner::AutoTuner>(state.range(2));

  std::vector<int> vec(SZ);
  std::iota(vec.begin(), vec.end(), 0);


  for (auto _ : state) {
    tuner::ATDriver AT;
    auto Tuner = easy::options::tuner_kind(TK);
    auto Range = IntRange(ISORT_MIN_CUTOFF, ISORT_MAX_CUTOFF, ISORT_IDEAL_CUTOFF);

    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      assert( isSorted(vec.data(), 0, vec.size()-1, int_cmp) );
      std::random_shuffle(vec.begin(), vec.end());
      benchmark::ClobberMemory();
      state.ResumeTiming();


      auto const& my_qsort = AT.reoptimize(Qsort, _1, _2, _3, int_cmp,
         Range, Tuner);

      my_qsort(vec.data(), 0, vec.size()-1);
    }
  }
}

static void BM_qsort(benchmark::State& state) {
  int SZ = state.range(0);
  const int ITERS = state.range(1);

  std::vector<int> vec(SZ);
  std::iota(vec.begin(), vec.end(), 0);

  for (auto _ : state) {
    for (int i = 0; i < ITERS; i++) {
      state.PauseTiming();
      assert( isSorted(vec.data(), 0, vec.size()-1, int_cmp) );
      std::random_shuffle(vec.begin(), vec.end());
      benchmark::ClobberMemory();
      state.ResumeTiming();

      Qsort(vec.data(), 0, vec.size()-1, int_cmp, ISORT_IDEAL_CUTOFF);
    }
  }
}


/////////////////////////////
// benchmark registration

#define QSORT_MIN 32768
#define QSORT_MAX 32768
#define ITER_MIN 128
#define ITER_MAX 1024

static void QSortArgs(benchmark::internal::Benchmark* b) {
  for (tuner::AutoTuner TK : tuner::AllTuners)
    for (int i = ITER_MIN; i <= ITER_MAX; i *= 2)
      for (int sz = QSORT_MIN; sz <= QSORT_MAX; sz *= 2)
        b->Args({sz, i, TK});
}

BENCHMARK(BM_qsort)
  ->Unit(benchmark::kMillisecond)
  ->RangeMultiplier(2)
  ->Ranges({{QSORT_MIN, QSORT_MAX}, {ITER_MIN, ITER_MAX}})
  ->UseRealTime();


BENCHMARK(BM_qsort_tuned)
  ->Unit(benchmark::kMillisecond)
  ->Apply(QSortArgs)
  ->UseRealTime();


#endif // BENCH_QSORT
