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

/* swap: interchange v[i] and v[j] */
void swap(int v[], int i, int j)
{
    int temp;
    temp = v[i];
    v[i] = v[j];
    v[j] = temp;
}

void isort(int v[], int sz, int (*cmp)(int, int)) {
  int i = 1;
  while (i < sz) {
    int j = i;
    while (j > 0 && cmp(v[j-1], v[j]) > 0) {
      swap(v, j-1, j);
      j--;
    }
    i++;
  }
}

// https://github.com/ctasims/The-C-Programming-Language--Kernighan-and-Ritchie/blob/master/ch04-functions-and-program-structure/qsort.c
// with modifications to support a cutoff to switch to insertion sort
void __attribute__((noinline)) Qsort(int v[], int left, int right, int (*cmp)(int, int), int cutOff)
{
    int i, last;

    if (left >= right)  // do nothing if array contains < 2 elems
        return;

    int sz = right - left;
    if (sz <= cutOff) {
      // use insertion sort to sort the entire array
      isort(v, sz, cmp);
      return;
    }

    // move partition elem to v[0]
    swap(v, left, (left + right)/2);
    last = left;

    for (i = left+1; i <= right; i++)  // partition
        if (cmp(v[i], v[left]) > 0)
            swap(v, ++last, i);

    swap(v, left, last);                // restore partition elem
    Qsort(v, left, last-1, cmp, cutOff);
    Qsort(v, last+1, right, cmp, cutOff);
}


/////////////////////////////////////////////////////////////////

#define ISORT_MAX_CUTOFF 200
#define ISORT_MIN_CUTOFF 0
#define ISORT_IDEAL_CUTOFF 30

using namespace tuned_param;

static void BM_qsort_jit_cache(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  static easy::Cache<> cache;

  for (auto _ : state) {
    state.PauseTiming();
    std::random_shuffle(vec.begin(), vec.end());
    benchmark::ClobberMemory();

    auto const& my_qsort = cache.jit(Qsort, _1, _2, _3, int_cmp, ISORT_IDEAL_CUTOFF);

    state.ResumeTiming();

    my_qsort(vec.data(), 0, vec.size()-1);

  }
}
BENCHMARK(BM_qsort_jit_cache)->RangeMultiplier(2)->Range(16,1024);



static void BM_qsort_tuned_bayes(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  static tuner::ATDriver AT;
  auto Tuner = easy::options::tuner_kind(tuner::AT_Bayes);

  for (auto _ : state) {
    state.PauseTiming();
    std::random_shuffle(vec.begin(), vec.end());
    benchmark::ClobberMemory();

    auto const& my_qsort = AT.reoptimize(Qsort, _1, _2, _3, int_cmp,
       IntRange(ISORT_MIN_CUTOFF, ISORT_MAX_CUTOFF, ISORT_IDEAL_CUTOFF), Tuner);

    state.ResumeTiming();

    my_qsort(vec.data(), 0, vec.size()-1);

  }

}
BENCHMARK(BM_qsort_tuned_bayes)->RangeMultiplier(2)->Range(16,1024);



static void BM_qsort_tuned_anneal(benchmark::State& state) {
  using namespace std::placeholders;
  int n = state.range(0);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  static tuner::ATDriver AT;
  auto Tuner = easy::options::tuner_kind(tuner::AT_Anneal);

  for (auto _ : state) {
    state.PauseTiming();
    std::random_shuffle(vec.begin(), vec.end());
    benchmark::ClobberMemory();

    auto const& my_qsort = AT.reoptimize(Qsort, _1, _2, _3, int_cmp,
       IntRange(ISORT_MIN_CUTOFF, ISORT_MAX_CUTOFF, ISORT_IDEAL_CUTOFF), Tuner);

    state.ResumeTiming();

    my_qsort(vec.data(), 0, vec.size()-1);

  }

}
BENCHMARK(BM_qsort_tuned_anneal)->RangeMultiplier(2)->Range(16,1024);



static void BM_qsort(benchmark::State& state) {
  int n = state.range(0);
  std::vector<int> vec(n);
  std::iota(vec.begin(), vec.end(), 0);

  for (auto _ : state) {
    state.PauseTiming();
    std::random_shuffle(vec.begin(), vec.end());
    benchmark::ClobberMemory();
    state.ResumeTiming();

    Qsort(vec.data(), 0, vec.size()-1, int_cmp, ISORT_IDEAL_CUTOFF);

  }
}
BENCHMARK(BM_qsort)->RangeMultiplier(2)->Range(16,1024);

#endif // BENCH_QSORT
