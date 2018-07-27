#ifndef TUNER_UTIL
#define TUNER_UTIL

#include <limits>

#define DEFAULT_STD_ERR_PCT 1.0

namespace tuner {
  // a "missing" value indicator
  static constexpr float MISSING = std::numeric_limits<float>::quiet_NaN();

}


#endif // TUNER_UTIL
