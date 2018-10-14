#ifndef TUNER_UTIL
#define TUNER_UTIL

#include <limits>
#include <cmath>
#include <random>
#include <bitset>

#define DEFAULT_MIN_TRIALS 2
#define DEFAULT_STD_ERR_PCT 10.0
// "HUNK" * 100 = percent
#define BEST_SWAP_MARGIN_HUNK 0.01
#define DEFAULT_COMPILE_AHEAD 10
#define EXPERIMENT_RATE 5
#define COMPILE_JOB_BAILOUT_MS 90'000

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

#include <fstream>
#include <string>

// quick and dirty JSON dumper
class JSON {
public:

  static void beginObject(std::ostream &file) {
    file << "{";
  }

  static void endObject(std::ostream &file) {
    file << "}";
  }

  static void beginArray(std::ostream &file) {
    file << "[";
  }

  static void endArray(std::ostream &file) {
    file << "]";
  }

  static void beginBind(std::ostream &file, std::string key) {
    fmt(file, key);
    file << " : ";
  }

  static void comma(std::ostream &file) {
    file << ",\n";
  }

  static void endBind(std::ostream &file) {
    comma(file);
  }

  static void fmt(std::ostream &file, std::string val) {
    file << "\"" << val << "\"";
  }

  static void fmt(std::ostream &file, const char *val) {
    file << "\"" << val << "\"";
  }

  template < typename ValTy >
  static void fmt(std::ostream &file, ValTy val) {
    file << std::to_string(val);
  }



  //////////////////////
  // common operations

  template < typename ValTy >
  static void output(std::ostream &file, std::string key, ValTy val) {
    beginBind(file, key);
    fmt(file, val);
    endBind(file);
  }

};


#endif // TUNER_UTIL
