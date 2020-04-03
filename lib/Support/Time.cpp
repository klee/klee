//===-- Time.cpp ----------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Support/ErrorHandling.h"
#include "klee/System/Time.h"


#include <cstdint>
#include <regex>
#include <sstream>
#include <tuple>
#include <sys/resource.h>


using namespace klee;


/* Why std::chrono:
 * - C++11
 * - separation between time points and durations
 * - good performance on Linux and macOS (similar to gettimeofday)
 * and not:
 * - clock_gettime(CLOCK_MONOTONIC_COARSE): software clock, Linux-specific
 * - clock_gettime(CLOCK_MONOTONIC): slowest on macOS, C-like API
 * - gettimeofday: C-like API, non-monotonic
 *
 * TODO: add time literals with C++14
 */

// === Point ===

// operators
time::Point& time::Point::operator+=(const time::Span &span) { point += span.duration; return *this; }
time::Point& time::Point::operator-=(const time::Span &span) { point -= span.duration; return *this; }

time::Point time::operator+(const time::Point &point, const time::Span &span) { return time::Point(point.point + span.duration); }
time::Point time::operator+(const time::Span &span, const time::Point &point) { return time::Point(point.point + span.duration); }
time::Point time::operator-(const time::Point &point, const time::Span &span) { return time::Point(point.point - span.duration); }
time::Span time::operator-(const time::Point &lhs, const time::Point &rhs) { return time::Span(lhs.point - rhs.point); }
bool time::operator==(const time::Point &lhs, const time::Point &rhs) { return lhs.point == rhs.point; }
bool time::operator!=(const time::Point &lhs, const time::Point &rhs) { return lhs.point != rhs.point; }
bool time::operator<(const time::Point &lhs, const time::Point &rhs) { return lhs.point < rhs.point; }
bool time::operator<=(const time::Point &lhs, const time::Point &rhs) { return lhs.point <= rhs.point; }
bool time::operator>(const time::Point &lhs, const time::Point &rhs) { return lhs.point > rhs.point; }
bool time::operator>=(const time::Point &lhs, const time::Point &rhs) { return lhs.point >= rhs.point; }


// === Span ===

// ctors
/// returns span from string in old (X.Y) and new (3h4min) format
time::Span::Span(const std::string &s) {
  if (s.empty()) return;

  std::regex re("^([0-9]*\\.?[0-9]+)|((([0-9]+)(h|min|s|ms|us|ns))+)$", std::regex::extended);
  std::regex nre("([0-9]+)(h|min|s|ms|us|ns)", std::regex::extended);
  std::smatch match;
  std::string submatch;

  // error
  if (!std::regex_match(s, match, re)) goto error;

  // old (double) format
  submatch = match[1].str();
  if (match[1].matched) {
    errno = 0;
    auto value = std::stod(submatch);
    if (errno) goto error;

    std::chrono::duration<double> d(value);
    duration = std::chrono::duration_cast<std::chrono::microseconds>(d);
  }

  // new (string) format
  submatch = match[2].str();
  for (std::smatch m; std::regex_search(submatch, m, nre); submatch = m.suffix()) {
    errno = 0;
    const auto value = std::stoull(m[1]);
    if (errno) goto error;

    Duration d;
    if (m[2] == "h") d = std::chrono::hours(value);
    else if (m[2] == "min") d = std::chrono::minutes(value);
    else if (m[2] ==   "s") d = std::chrono::seconds(value);
    else if (m[2] ==  "ms") d = std::chrono::milliseconds(value);
    else if (m[2] ==  "us") d = std::chrono::microseconds(value);
    else if (m[2] ==  "ns") d = std::chrono::nanoseconds(value);
    else goto error;

    duration += d;
  }

  return;

error:
  klee_error("Illegal number format: %s", s.c_str());
}

// operators
time::Span& time::Span::operator=(const time::Duration &d) { duration = d; return *this; };
time::Span& time::Span::operator+=(const time::Span &other) { duration += other.duration; return *this; }
time::Span& time::Span::operator-=(const time::Span &other) { duration -= other.duration; return *this; }
time::Span& time::Span::operator*=(unsigned factor) { duration *= factor; return *this; }
time::Span& time::Span::operator*=(double factor) {
  duration = std::chrono::microseconds((std::uint64_t)((double)toMicroseconds() * factor));
  return *this;
}

time::Span time::operator+(const time::Span &lhs, const time::Span &rhs) { return time::Span(lhs.duration + rhs.duration); }
time::Span time::operator-(const time::Span &lhs, const time::Span &rhs) { return time::Span(lhs.duration - rhs.duration); }
time::Span time::operator*(const time::Span &span, double factor) {
  return time::Span(std::chrono::microseconds((std::uint64_t)((double)span.toMicroseconds() * factor)));
};
time::Span time::operator*(double factor, const time::Span &span) {
  return time::Span(std::chrono::microseconds((std::uint64_t)((double)span.toMicroseconds() * factor)));
};
time::Span time::operator*(const time::Span &span, unsigned factor) { return time::Span(span.duration * factor); }
time::Span time::operator*(unsigned factor, const time::Span &span) { return time::Span(span.duration * factor); }
time::Span time::operator/(const time::Span &span, unsigned divisor) { return time::Span(span.duration / divisor); }
bool time::operator==(const time::Span &lhs, const time::Span &rhs) { return lhs.duration == rhs.duration; }
bool time::operator<=(const time::Span &lhs, const time::Span &rhs) { return lhs.duration <= rhs.duration; }
bool time::operator>=(const time::Span &lhs, const time::Span &rhs) { return lhs.duration >= rhs.duration; }
bool time::operator<(const time::Span &lhs, const time::Span &rhs) { return lhs.duration < rhs.duration; }
bool time::operator>(const time::Span &lhs, const time::Span &rhs) { return lhs.duration > rhs.duration; }

std::ostream& time::operator<<(std::ostream &stream, time::Span span) { return stream << span.toSeconds() << 's'; }
llvm::raw_ostream& time::operator<<(llvm::raw_ostream &stream, time::Span span) { return stream << span.toSeconds() << 's'; }


// units
time::Span time::hours(std::uint16_t ticks) { return time::Span(std::chrono::hours(ticks)); }
time::Span time::minutes(std::uint16_t ticks) { return time::Span(std::chrono::minutes(ticks)); }
time::Span time::seconds(std::uint64_t ticks) { return time::Span(std::chrono::seconds(ticks)); }
time::Span time::milliseconds(std::uint64_t ticks) { return time::Span(std::chrono::milliseconds(ticks)); }
time::Span time::microseconds(std::uint64_t ticks) { return time::Span(std::chrono::microseconds(ticks)); }
time::Span time::nanoseconds(std::uint64_t ticks) { return time::Span(std::chrono::nanoseconds(ticks)); }


// conversions
time::Span::operator time::Duration() const { return duration; }

time::Span::operator bool() const { return duration.count() != 0; }

time::Span::operator timeval() const {
  timeval tv{};
  const auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration);
  const auto usecs = std::chrono::duration_cast<std::chrono::microseconds>(duration - secs);
  tv.tv_sec = secs.count();
  tv.tv_usec = usecs.count();
  return tv;
}

std::uint64_t time::Span::toMicroseconds() const {
  return (std::uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

double time::Span::toSeconds() const {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() / (double)1000000000;
}

std::tuple<std::uint32_t, std::uint8_t, std::uint8_t> time::Span::toHMS() const {
  auto d = duration;
  const auto h = std::chrono::duration_cast<std::chrono::hours>(d);
  const auto m = std::chrono::duration_cast<std::chrono::minutes>(d -= h);
  const auto s = std::chrono::duration_cast<std::chrono::seconds>(d -= m);

  return std::make_tuple((std::uint32_t) h.count(), (std::uint8_t) m.count(), (std::uint8_t) s.count());
}


// methods
/// Returns information about clock
std::string time::getClockInfo() {
  std::stringstream buffer;
  buffer << "Using monotonic steady clock with "
         << std::chrono::steady_clock::period::num
         << '/'
         << std::chrono::steady_clock::period::den
         << "s resolution\n";
  return buffer.str();
}


/// Returns time spent by this process in user mode
time::Span time::getUserTime() {
  rusage usage{};
  auto ret = ::getrusage(RUSAGE_SELF, &usage);

  if (ret) {
    klee_warning("getrusage returned with error, return (0,0)");
    return {};
  } else {
    return time::seconds(static_cast<std::uint64_t>(usage.ru_utime.tv_sec)) +
           time::microseconds(static_cast<std::uint64_t>(usage.ru_utime.tv_usec));
  }
}


/// Returns point in time using a monotonic steady clock
time::Point time::getWallTime() {
  return time::Point(std::chrono::steady_clock::now());
}

