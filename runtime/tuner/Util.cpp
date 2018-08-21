
#include <tuner/Util.h>

#include <chrono>
#include <thread>

namespace tuner {

int pow2Bit(uint64_t val) {
  // NOTE: if we set the 5th bit, then:
  // 0b100000 - 1 = 0b011111, and then
  // popcnt(0b011111) = 5

  if (val == 1)
    return 0;

  val -= 1;
  std::bitset<16> bits(val);
  return bits.count();
}

void sleep_for(unsigned ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // end namespace
