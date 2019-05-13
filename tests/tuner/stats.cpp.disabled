// RUN: %atjitc   %s -o %t
// RUN: %t > %t.json
// RUN: %jsonlint < %t.json

#include <tuner/driver.h>
#include <tuner/param.h>

#include <thread>

using namespace std::placeholders;
using namespace tuned_param;
using namespace easy::options;

int collatz(int seed) {
  int n = seed;
  int steps = 0;

  while(n != 1) {

    if (n % 2 == 0)
      n /= 2;
    else
      n = 3*n + 1;

    steps += 1;
  }

  return steps;
}

int main(int argc, char** argv) {

  tuner::AutoTuner TunerKind = tuner::AT_Random;
  const int ITERS = 1000;
  tuner::ATDriver AT;

  // we pick 6171 because it's the one generating the most steps
  // for all values less than it.
  // see: https://oeis.org/A006877

  for (int i = 0; i < ITERS; i++) {
    auto const &OptimizedFun = AT.reoptimize(collatz,
          IntRange(1, 65536, 6171),
          tuner_kind(TunerKind),
          blocking(true));

    OptimizedFun();
  }

  AT.exportStats();

  return 0;
}
