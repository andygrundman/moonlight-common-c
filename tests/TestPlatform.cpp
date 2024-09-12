#include <gtest/gtest.h>

extern "C" {
  #include "Limelight-internal.h"
}

// Yes, this is dumb and I should use a mock timer.
// They are enabled for now to test the other platforms.

static bool within_n(int limit, uint64_t a, uint64_t b) {
  if (abs((int64_t)(a - b)) <= limit) {
    return true;
  }
  return false;
}

static bool within_one(uint64_t a, uint64_t b) {
  return within_n(1, a, b);
}

TEST(Platform, Init) {
  ASSERT_EQ(initializePlatform(), 0);
}

TEST(Platform, GetTicks64_ms) {
  // make sure we've been running long enough to have meaningful results
  while (Plt_GetTicks64_ms() < 5) {
    PltSleepMs(1);
  }

  // allow the values to differ by at most 1
  EXPECT_TRUE( within_one(Plt_GetTicks64(), Plt_GetTicks64_ms()) ) << "Plt_GetTicks64() == Plt_GetTicks64_ms()";
  EXPECT_TRUE( within_one(Plt_GetTicks64(), LiGetMillis()) )       << "Plt_GetTicks64() == LiGetMillis()";

  // this should lose precision, with us being truncated to the same value by integer division
  EXPECT_TRUE( within_one(Plt_GetTicks64_ms(), Plt_GetTicks64_us() / 1000) ) << "_us() can be truncated to equal _ms()";
}

TEST(Platform, GetTicks64_us) {
  // make sure we've been running long enough to have meaningful results
  while (Plt_GetTicks64_us() < 5000) {
    PltSleepMs(1);
  }

  EXPECT_TRUE( within_n(1000, Plt_GetTicks64_us(), LiGetMicroseconds()) )     << "Plt_GetTicks64_us() == LiGetMicroseconds()";
  EXPECT_FALSE( within_one(Plt_GetTicks64_us(), Plt_GetTicks64_ms() * 1000) ) << "Plt_GetTicks64_ms() is lower precision than _us()";
}

TEST(Platform, Cleanup) {
  cleanupPlatform();
  SUCCEED();
}
