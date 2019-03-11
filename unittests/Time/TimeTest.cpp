#include "klee/Internal/System/Time.h"
#include "gtest/gtest.h"
#include "gtest/gtest-death-test.h"

#include <cerrno>
#include <sstream>


int finished = 0;

using namespace klee;


TEST(TimeTest, TimingFunctions) {
  auto t = time::getClockInfo();
  ASSERT_STRNE(t.c_str(), "");

  auto p0 = time::getWallTime();
  auto p1 = time::getWallTime();
  ASSERT_GT(p0, time::Point());
  ASSERT_GT(p1, time::Point());
  ASSERT_LE(p0, p1);

  time::getUserTime();
}


TEST(TimeTest, TimeParserNewFormat) {
  // valid
  errno = 0;
  auto s0 = time::Span("");
  ASSERT_EQ(s0, time::Span());
  ASSERT_EQ(errno, 0);

  s0 = time::Span("3h10min");
  ASSERT_EQ(s0, time::minutes(190));
  s0 = time::Span("5min5min");
  ASSERT_EQ(s0, time::seconds(600));
  s0 = time::Span("3us");
  ASSERT_EQ(s0, time::microseconds(3));
  s0 = time::Span("1h1min1s1ms1us1ns");
  ASSERT_EQ(s0, time::nanoseconds(3661001001001));
  s0 = time::Span("1min1min");
  ASSERT_EQ(s0, time::minutes(2));

  // invalid
  ASSERT_EXIT(time::Span("h"), ::testing::ExitedWithCode(1), "Illegal number format: h");
  ASSERT_EXIT(time::Span("-5min"), ::testing::ExitedWithCode(1), "Illegal number format: -5min");
  ASSERT_EXIT(time::Span("3.5h"), ::testing::ExitedWithCode(1), "Illegal number format: 3.5h");
  ASSERT_EXIT(time::Span("3mi"), ::testing::ExitedWithCode(1), "Illegal number format: 3mi");
}


TEST(TimeTest, TimeParserOldFormat) {
  // valid
  errno = 0;

  auto s0 = time::Span("20");
  ASSERT_EQ(s0, time::seconds(20));
  s0 = time::Span("3.5");
  ASSERT_EQ(s0, time::milliseconds(3500));
  s0 = time::Span("0.0");
  ASSERT_EQ(s0, time::Span());
  s0 = time::Span("0");
  ASSERT_EQ(s0, time::Span());

  ASSERT_EQ(errno, 0);

  // invalid
  ASSERT_EXIT(time::Span("-3.5"), ::testing::ExitedWithCode(1), "Illegal number format: -3.5");
  ASSERT_EXIT(time::Span("NAN"), ::testing::ExitedWithCode(1), "Illegal number format: NAN");
  ASSERT_EXIT(time::Span("INF"), ::testing::ExitedWithCode(1), "Illegal number format: INF");
  ASSERT_EXIT(time::Span("foo"), ::testing::ExitedWithCode(1), "Illegal number format: foo");
}


TEST(TimeTest, TimeArithmeticAndComparisons) {
  auto h   = time::hours(1);
  auto min = time::minutes(1);
  auto sec = time::seconds(1);
  auto ms  = time::milliseconds(1);
  auto us  = time::microseconds(1);
  auto ns  = time::nanoseconds(1);

  ASSERT_GT(h, min);
  ASSERT_GT(min, sec);
  ASSERT_GT(sec, ms);
  ASSERT_GT(ms, us);
  ASSERT_GT(us, ns);

  ASSERT_LT(min, h);
  ASSERT_LT(sec, min);
  ASSERT_LT(ms, sec);
  ASSERT_LT(us, ms);
  ASSERT_LT(ns, us);

  ASSERT_GE(h, min);
  ASSERT_LE(min, h);

  auto sec2 = time::seconds(1);
  ASSERT_EQ(sec, sec2);
  sec2 += time::nanoseconds(1);
  ASSERT_LT(sec, sec2);

  auto op0 = time::seconds(1);
  auto op1 = op0 / 1000U;
  ASSERT_EQ(op1, ms);
  op0 = time::nanoseconds(3);
  op1 = op0 * 1000U;
  ASSERT_EQ(op1, 3U*us);

  op0 = (time::seconds(10) + time::minutes(1) - time::milliseconds(10000)) * 60U;
  ASSERT_EQ(op0, h);

  auto p1 = time::getWallTime();
  auto p2 = p1;
  p1 += time::seconds(10);
  p2 -= time::seconds(15);
  ASSERT_EQ(p1 - p2, time::seconds(25));

  auto s = time::minutes(3);
  p1 = s + p2;
  ASSERT_NE(p1, p2);
  ASSERT_LT(p2, p1);
  p1 = p1 - s;
  ASSERT_EQ(p1, p2);

  s = time::minutes(5);
  s -= time::minutes(4);
  ASSERT_EQ(s, time::seconds(60));
}


TEST(TimeTest, TimeConversions) {
  auto t = time::Span("3h14min1s");
  auto d = t.toSeconds();
  ASSERT_EQ(d, 11641.0);

  std::uint32_t h;
  std::uint8_t m, s;
  std::tie(h, m, s) = t.toHMS();
  ASSERT_EQ(h, 3u);
  ASSERT_EQ(m, 14u);
  ASSERT_EQ(s, 1u);

  ASSERT_TRUE((bool)t);
  ASSERT_FALSE((bool)time::Span());

  auto us = t.toMicroseconds();
  ASSERT_EQ(us, 11641000000u);

  t += time::microseconds(42);
  auto v = (timeval)t;
  ASSERT_EQ(v.tv_sec, 11641);
  ASSERT_EQ(v.tv_usec, 42);

  t = std::chrono::seconds(1);
  ASSERT_EQ(t, time::seconds(1));
  auto u = (std::chrono::steady_clock::duration)t;
  ASSERT_EQ(u, std::chrono::seconds(1));

  std::ostringstream os;
  os << time::Span("2.5");
  ASSERT_EQ(os.str(), "2.5s");
}

TEST(TimeTest, ImplicitArithmeticConversions) {
  auto t1 = time::Span("1000s");
  t1 *= 2U;
  auto d = t1.toSeconds();
  ASSERT_EQ(d, 2000.0);

  auto t2 = t1 * 1.5;
  d = t2.toSeconds();
  ASSERT_EQ(d, 3000.0);

  t2 = 2.5 * t1;
  d = t2.toSeconds();
  ASSERT_EQ(d, 5000.0);

  t1 = time::Span("1000s");
  t1 *= 2.2;
  d = t1.toSeconds();
  ASSERT_EQ(d, 2200.0);
}