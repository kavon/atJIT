
#include <tuner/LoopKnob.h>

namespace tuner {

template < typename RNE >  // RandomNumberEngine
LoopSetting genRandomLoopSetting(RNE &Eng) {
  LoopSetting LS;

  {
    /////////////////////////
    // UNROLLING KNOBS

    // roll a 100-sided die, so we can make a weighted choice.
    std::uniform_int_distribution<unsigned> diceRoll(0, 100);
    unsigned unrollChoice = diceRoll(Eng);
    if (unrollChoice <= 10) {
      LS.UnrollDisable = true;
    } else if (unrollChoice <= 50) {
      LS.UnrollFull = true;
    } else {
      // pick a power-of-two (>= 2) unrolling factor
      std::uniform_int_distribution<unsigned> bitNum(1, 12); // 2^12 = 4096
      LS.UnrollCount = ((uint16_t) 1) << bitNum(Eng);
    }
  }


  {
    /////////////////////////
    // VECTORIZATION KNOBS (AND RELATED TO VECTORIZATION)

    std::uniform_int_distribution<unsigned> diceRoll1(0, 100);
    unsigned vecChoice = diceRoll1(Eng);

    if (vecChoice <= 30) {
      LS.VectorizeEnable = false;
    } else if (vecChoice <= 75) {
      LS.VectorizeEnable = true;
    } // otherwise, we don't specify anything.

    if (LS.VectorizeEnable && LS.VectorizeEnable.value()) {
      std::uniform_int_distribution<unsigned> diceRoll2(0, 100);
      // should we specify a width? otherwise, LLVM chooses automatically.
      if (diceRoll2(Eng) >= 40) {
        // pick a power-of-two (>= 2) width
        std::uniform_int_distribution<unsigned> bitNum(1, 5); // 2^5 = 32
        LS.VectorizeWidth = ((uint16_t) 1) << bitNum(Eng);
      }
    }

    std::uniform_int_distribution<unsigned> diceRoll2(0, 100);
    unsigned interleaveChoice = diceRoll2(Eng);

    if (interleaveChoice < 65) {

      if (interleaveChoice < 10) {
        LS.InterleaveCount = 0; // disable
      } else if (interleaveChoice < 35) {
        LS.InterleaveCount = 1; // automatic
      } else {
        // pick a power-of-two (>= 2) count
        std::uniform_int_distribution<unsigned> bitNum(1, 6); // 2^6 = 64
        LS.InterleaveCount = ((uint16_t) 1) << bitNum(Eng);
      }

    }


    std::uniform_int_distribution<unsigned> diceRoll3(0, 100);
    unsigned distributeChoice = diceRoll3(Eng);

    if (distributeChoice <= 50) {
        LS.Distribute = distributeChoice <= 25;
    }

  }


  {
    /////////////////////////
    // LICM KNOBS

    std::uniform_int_distribution<unsigned> diceRoll(0, 100);
    unsigned choice = diceRoll(Eng);

    if (choice < 30) {
      LS.LICMVerDisable = true;
    } else if (choice < 80) {
      LS.LICMVerDisable = false; // will try removing it if it was disabled.
    }
  }

  return LS;
}

// specializations go here.

template LoopSetting genRandomLoopSetting<std::mt19937>(std::mt19937&);

} // end namespace
