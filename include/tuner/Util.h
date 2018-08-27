#ifndef TUNER_UTIL
#define TUNER_UTIL

#include <limits>
#include <cmath>
#include <random>
#include <bitset>

#define DEFAULT_MIN_TRIALS 2
#define DEFAULT_STD_ERR_PCT 5.0
#define DEFAULT_COMPILE_AHEAD 10
#define EXPERIMENT_RATE 10
#define COMPILE_JOB_BAILOUT_MS 120'000

namespace tuner {
  // a "missing" value indicator
  static constexpr float MISSING = std::numeric_limits<float>::quiet_NaN();

  /////////
  // generates a random integer that is "nearby" an
  // existing integer, within the given inclusive range
  // [min, max], given the amount of energy we have
  // to move away from the current integer.
  // Energy must be within [0, 100].
  //
  // NOTE: the returned integer may be equal to the existing one.
  template < typename RNE >
  int nearbyInt (RNE &Eng, int cur, int min, int max, double energy) {
    // 68% of values drawn will be within this distance from the old value.
    int scaledRange = (max - min) * (energy / 100.0);
    int stdDev = scaledRange / 2.0;

    // sample from a normal distribution, where the mean is
    // the old value, and the std deviation is influenced by the energy.
    // NOTE: a logistic distribution, which is like a higher kurtosis
    // normal distribution, might give us better numbers when the
    // energy is low?
    std::normal_distribution<double> dist(cur, stdDev);

    // ensure the value is in the right range.
    int val = std::round(dist(Eng));
    val = std::max(val, min);
    val = std::min(val, max);

    return val;
  }

  // log_2(val) where val is a power-of-two.
  int pow2Bit(uint64_t val);

  // sleeps the current thread
  void sleep_for(unsigned ms);

}


#endif // TUNER_UTIL
