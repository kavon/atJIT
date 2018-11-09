#ifndef BENCH_SPECNORM
#define BENCH_SPECNORM

/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Contributed by Sebastien Loisel
 * Adapted by Kavon Farvardin
 */

 ///////////////////////////
 // benchmark code

#include <cstdlib>
#include <cmath>

double eval_A(int i, int j) { return 1.0/((i+j)*(i+j+1)/2+i+1); }

void eval_A_times_u(int N, const double u[], double Au[])
{
  int i,j;
  for(i=0;i<N;i++)
    {
      Au[i]=0;
      for(j=0;j<N;j++) Au[i]+=eval_A(i,j)*u[j];
    }
}

void eval_At_times_u(int N, const double u[], double Au[])
{
  int i,j;
  for(i=0;i<N;i++)
    {
      Au[i]=0;
      for(j=0;j<N;j++) Au[i]+=eval_A(j,i)*u[j];
    }
}

void eval_AtA_times_u(int N, const double u[], double AtAu[]) {
  double* v = (double*) malloc(sizeof(double) * N);
  eval_A_times_u(N,u,v);
  eval_At_times_u(N,v,AtAu);
  free(v);
}


/////////////////////////////////////////////////
// start of benchmark. default N is 2000
//
// for N = 100, output should be 1.274219991
double __attribute__((noinline)) spectralnorm (const int N) {
  int i;
  double* u = (double*) malloc(sizeof(double) * N);
  double* v = (double*) malloc(sizeof(double) * N);
  double vBv,vv;
  for(i=0;i<N;i++) u[i]=1;
  for(i=0;i<10;i++)
    {
      eval_AtA_times_u(N,u,v);
      eval_AtA_times_u(N,v,u);
    }
  vBv=vv=0;
  for(i=0;i<N;i++) { vBv+=u[i]*v[i]; vv+=v[i]*v[i]; }
  free(u);
  free(v);
  return sqrt(vBv/vv);
}

/////////////////////////////////////////////////////////////////


/////////////////////
// benchmark driver

// TUNED, with all JIT overheads included.
static void TUNING_spectralnorm(benchmark::State& state) {
  const int N = state.range(0);
  const int ITERS = state.range(1);
  tuner::AutoTuner TK = static_cast<tuner::AutoTuner>(state.range(2));

  for (auto _ : state) {
    tuner::ATDriver AT("./TUNING_spectralnorm.json");
    auto Tuner = easy::options::tuner_kind(TK);

    for (int i = 0; i < ITERS; i++) {
      auto const& my_specnorm = AT.reoptimize(spectralnorm, _1, Tuner);

      my_specnorm(N);
    }
    // NOTE: we don't want to time the driver's destructor
    state.PauseTiming();
  }
}


/////////////////////////////
// benchmark registration

#define SPECNORM_MIN 150
#define SPECNORM_MAX 300
#define ITER_MIN 40
#define ITER_MAX 80

static void SpecnormArgs(benchmark::internal::Benchmark* b) {
  for (tuner::AutoTuner TK : tuner::AllTuners)
    for (int i = ITER_MIN; i <= ITER_MAX; i *= 2)
      for (int sz = SPECNORM_MIN; sz <= SPECNORM_MAX; sz *= 2)
        b->Args({sz, i, TK});
}

BENCHMARK(TUNING_spectralnorm)
  ->Unit(benchmark::kMillisecond)
  ->Apply(SpecnormArgs)
  ->UseRealTime();


// cleanup
#undef SPECNORM_MIN
#undef SPECNORM_MAX
#undef ITER_MIN
#undef ITER_MAX


#endif // BENCH_SPECNORM
