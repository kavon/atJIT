
#include <tuner/LoopKnob.h>

namespace tuner {

//////
// NOTE: all of these settings must form a contiguous int range.
// and must logically convey the distance between settings.

// CNT is in the range [2^min, 2^max]
static constexpr int UNROLL_CNT_MIN = 1;
static constexpr int UNROLL_CNT_MAX = 6; // 2 ^ 6 = 64

static constexpr int UNROLL_DISABLE = UNROLL_CNT_MIN-2;
static constexpr int UNROLL_MISSING = UNROLL_CNT_MIN-1;
static constexpr int UNROLL_FULL = UNROLL_CNT_MAX+1;

// NOTE: I don't see a reason to generate code that disables unrolling.
// It's better to just not say anything about unrolling.
//
// Plus, it messes with how I have setup the exponential distribution
// since we don't want to have a high chance of turning unrolling off.
static constexpr int UNROLL_MAX = UNROLL_FULL;
static constexpr int UNROLL_MIN = UNROLL_MISSING; // NOTE see above.
static constexpr int UNROLL_DFLT = 2; // 2 ^ 2 = 4


int unrollAsInt(LoopSetting const& LS) {
  bool count = LS.UnrollCount.has_value();
  bool disable = LS.UnrollDisable.has_value();
  bool full = LS.UnrollFull.has_value();

  assert(count + disable + full <= 1
         && "only one may be set");

  if (disable)
    return UNROLL_DISABLE;
  else if (full)
    return UNROLL_FULL;
  else if (count)
    return pow2Bit(LS.UnrollCount.value());

  return UNROLL_MISSING;
}

void setUnroll(LoopSetting &LS, int v) {
  assert(v >= UNROLL_MIN && v <= UNROLL_MAX);
  LS.UnrollCount = std::nullopt;
  LS.UnrollDisable = std::nullopt;
  LS.UnrollFull = std::nullopt;

  switch (v) {
    case UNROLL_MISSING: {
      // already reset.
    } break;

    case UNROLL_DISABLE: {
      LS.UnrollDisable = true;
    } break;

    case UNROLL_FULL: {
      LS.UnrollFull = true;
    } break;

    default: {
      assert(v >= UNROLL_CNT_MIN && v <= UNROLL_CNT_MAX);
      LS.UnrollCount = ((uint16_t) 1) << v;
    } break;
  };
}




// width is such that [2^min, 2^max]
static constexpr int VEC_WIDTH_MAX = 5; // 2^5 = 32
static constexpr int VEC_WIDTH_MIN = 0; // 2^0 = 1 => "disable"
static constexpr int VEC_MISSING = VEC_WIDTH_MAX+1;

// [disable, ... widths ..., automatic, aka, missing]
static constexpr int VEC_MIN = VEC_WIDTH_MIN;
static constexpr int VEC_MAX = VEC_MISSING;

int vecAsInt(LoopSetting const& LS) {
  if (LS.VectorizeWidth.has_value())
    return pow2Bit(LS.VectorizeWidth.value());

  return VEC_MISSING;
}

void setVec(LoopSetting &LS, int v) {
  assert(v >= VEC_MIN && v <= VEC_MAX);

  switch (v) {
    case VEC_MISSING: {
      LS.VectorizeWidth = std::nullopt;
    } break;

    default: {
      LS.VectorizeWidth = ((uint16_t) 1) << v;
    } break;
  };
}




// count is such that [2^min, 2^max]
static constexpr int INTRLV_COUNT_MAX = 6; // 2^6 = 64
static constexpr int INTRLV_COUNT_MIN = 0; // 2^0 = 1 => "disable"
static constexpr int INTRLV_MISSING = INTRLV_COUNT_MAX+1;

// [disable, ... counts ..., automatic, aka, missing]
static constexpr int INTRLV_MIN = INTRLV_COUNT_MIN;
static constexpr int INTRLV_MAX = INTRLV_MISSING;

int interleaveAsInt(LoopSetting const& LS) {
  if (LS.InterleaveCount.has_value())
    return pow2Bit(LS.InterleaveCount.value());

  return INTRLV_MISSING;
}

void setInterleave(LoopSetting &LS, int v) {
  assert(v >= INTRLV_MIN && v <= INTRLV_MAX);

  switch (v) {
    case INTRLV_MISSING: {
      // assuming that no specifier implies "automatic"
      LS.InterleaveCount = std::nullopt;
    } break;

    default: {
      LS.InterleaveCount = ((uint16_t) 1) << v;
    } break;
  };
}




// [disabled, missing, enabled]
static constexpr int FLAG_TRUE = 2;
static constexpr int FLAG_MISSING = 1;
static constexpr int FLAG_FALSE = 0;

static constexpr int FLAG_MIN = FLAG_FALSE;
static constexpr int FLAG_MAX = FLAG_TRUE;

int boolOptAsInt(std::optional<bool> &Opt) {
  if (Opt.has_value()) {
    return (Opt.value() ? FLAG_TRUE : FLAG_FALSE);
  }
  return FLAG_MISSING;
}

void setBoolOpt(std::optional<bool> &Opt, int v) {
  assert(v >= FLAG_MIN && v <= FLAG_MAX);

  if (v == FLAG_MISSING) {
    Opt = std::nullopt;
    return;
  }

  Opt = FLAG_TRUE ? true : false;

}




static constexpr int SECTION_SIZE_MAX = 128;
static constexpr int SECTION_SIZE_MIN = 2;
static constexpr int SECTION_MISSING = SECTION_SIZE_MIN-1;

// [missing, ... sizes ...]
static constexpr int SECTION_MIN = SECTION_MISSING;
static constexpr int SECTION_MAX = SECTION_SIZE_MAX;

int sectionAsInt(LoopSetting const& LS) {
  if (LS.Section.has_value())
    return LS.Section.value();

  return SECTION_MISSING;
}

void setSection(LoopSetting &LS, int v) {
  assert(v >= SECTION_MIN && v <= SECTION_MAX);

  switch (v) {
    case SECTION_MISSING: {
      LS.Section = std::nullopt;
    } break;

    default: {
      LS.Section = v;
    } break;
  };
}

// Roughly corresponds to flipping a coin with these properties:
//
// P(true) = trueBias / 100
// P(false) = 1 - P(true)
template < typename RNE >
bool biasedFlip(int trueBias, RNE &Eng) {
  assert(trueBias >= 0 && trueBias <= 100);
  std::uniform_int_distribution<int> dist(1, 99);
  return dist(Eng) <= trueBias;
}


//////////////////////////////////////////////
//////////////////////////////////////////////


template < typename RNE >
LoopSetting genNearbyLoopSetting(RNE &Eng, LoopSetting LS, double energy) {

    setUnroll(LS, nearbyInt(Eng,
      unrollAsInt(LS), UNROLL_MIN, UNROLL_MAX, energy));

    setVec(LS, nearbyInt(Eng,
      vecAsInt(LS), VEC_MIN, VEC_MAX, energy));

    setInterleave(LS, nearbyInt(Eng,
      interleaveAsInt(LS), INTRLV_MIN, INTRLV_MAX, energy));

    setBoolOpt(LS.Distribute, nearbyInt(Eng,
      boolOptAsInt(LS.Distribute), FLAG_MIN, FLAG_MAX, energy));

    setBoolOpt(LS.LICMVerDisable, nearbyInt(Eng,
      boolOptAsInt(LS.LICMVerDisable), FLAG_MIN, FLAG_MAX, energy));

#ifdef POLLY_KNOBS

    setSection(LS, nearbyInt(Eng,
      sectionAsInt(LS), SECTION_MIN, SECTION_MAX, energy));

#endif

  return LS;
}


template < typename RNE >  // RandomNumberEngine
LoopSetting genRandomLoopSetting(RNE &Eng) {
  LoopSetting LS;

  if (biasedFlip(50, Eng)) { // UNROLLING

    // NOTE: we use an exponential distribution whose mean is
    // at a reasonable unrolling factor, because it should be rare
    // that we end up asking to do a full unroll.
    // mean of exp dist = 1 / lambda.

    double lambda = 1.0 / (UNROLL_DFLT - UNROLL_MIN);
    std::exponential_distribution<double> dist(lambda);

    int point = std::round(dist(Eng));

    // we shift the point generated to start at our min, instead of 0
    int val = UNROLL_MIN + point;
    val = std::min(val, UNROLL_MAX);
    val = std::max(val, UNROLL_MIN);

    setUnroll(LS, val);
  }

  if (biasedFlip(50, Eng)) { // VECTORIZE WIDTH
    std::uniform_int_distribution<int> dist(VEC_MIN, VEC_MAX);
    setVec(LS, dist(Eng));
  }

  if (biasedFlip(50, Eng)) { // INTERLEAVE COUNT
    std::uniform_int_distribution<int> dist(INTRLV_MIN, INTRLV_MAX);
    setInterleave(LS, dist(Eng));
  }

  if (biasedFlip(50, Eng)) { // LOOP DISTRIBUTE
    std::uniform_int_distribution<int> dist(FLAG_MIN, FLAG_MAX);
    setBoolOpt(LS.Distribute, dist(Eng));
  }

  if (biasedFlip(50, Eng)) { // LICM VERSIONING
    std::uniform_int_distribution<int> dist(FLAG_MIN, FLAG_MAX);
    setBoolOpt(LS.LICMVerDisable, dist(Eng));
  }

#ifdef POLLY_KNOBS

  if (biasedFlip(75, Eng)) { // LOOP SECTIONING
    std::uniform_int_distribution<int> dist(SECTION_MIN, SECTION_MAX);
    setSection(LS, dist(Eng));
  }

#endif

  return LS;
}

// specializations go here.

template
LoopSetting genRandomLoopSetting<std::mt19937_64>(std::mt19937_64&);

template
LoopSetting genNearbyLoopSetting<std::mt19937_64>(std::mt19937_64&, LoopSetting, double);

} // end namespace
