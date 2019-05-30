//===-- Time.h --------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TIME_H
#define KLEE_TIME_H

#include "llvm/Support/raw_ostream.h"

#include <chrono>
#include <string>
#include <sys/time.h>

namespace klee {
  namespace time {

    /// The klee::time namespace offers various functions to measure the time (`getWallTime`)
    /// and to get timing information for the current KLEE process (`getUserTime`).
    /// This implementation is based on `std::chrono` and uses time points and time spans.
    /// For KLEE statistics, spans are converted to µs and stored in `uint64_t`.

    struct Point;
    struct Span;

    /// Returns information about clock
    std::string getClockInfo();

    /// Returns time spent by this process in user mode
    Span getUserTime();

    /// Returns point in time using a monotonic steady clock
    Point getWallTime();

    struct Point {
      using SteadyTimePoint = std::chrono::steady_clock::time_point;

      SteadyTimePoint point;

      // ctors
      Point() = default;
      explicit Point(SteadyTimePoint p): point(p) {};

      // operators
      Point& operator+=(const Span&);
      Point& operator-=(const Span&);
    };

    // operators
    Point operator+(const Point&, const Span&);
    Point operator+(const Span&, const Point&);
    Point operator-(const Point&, const Span&);
    Span operator-(const Point&, const Point&);
    bool operator==(const Point&, const Point&);
    bool operator!=(const Point&, const Point&);
    bool operator<(const Point&, const Point&);
    bool operator<=(const Point&, const Point&);
    bool operator>(const Point&, const Point&);
    bool operator>=(const Point&, const Point&);

    namespace { using Duration = std::chrono::steady_clock::duration; }

    struct Span {
      Duration duration = Duration::zero();

      // ctors
      Span() = default;
      explicit Span(const Duration &d): duration(d) {}
      explicit Span(const std::string &s);

      // operators
      Span& operator=(const Duration&);
      Span& operator+=(const Span&);
      Span& operator-=(const Span&);
      Span& operator*=(unsigned);
      Span& operator*=(double);

      // conversions
      explicit operator Duration() const;
      explicit operator bool() const;
      explicit operator timeval() const;

      std::uint64_t toMicroseconds() const;
      double toSeconds() const;
      std::tuple<std::uint32_t, std::uint8_t, std::uint8_t> toHMS() const; // hours, minutes, seconds
    };

    Span operator+(const Span&, const Span&);
    Span operator-(const Span&, const Span&);
    Span operator*(const Span&, double);
    Span operator*(double, const Span&);
    Span operator*(const Span&, unsigned);
    Span operator*(unsigned, const Span&);
    Span operator/(const Span&, unsigned);
    bool operator==(const Span&, const Span&);
    bool operator<=(const Span&, const Span&);
    bool operator>=(const Span&, const Span&);
    bool operator<(const Span&, const Span&);
    bool operator>(const Span&, const Span&);

    /// Span -> "X.Ys"
    std::ostream& operator<<(std::ostream&, Span);
    llvm::raw_ostream& operator<<(llvm::raw_ostream&, Span);

    /// time spans
    Span hours(std::uint16_t);
    Span minutes(std::uint16_t);
    Span seconds(std::uint64_t);
    Span milliseconds(std::uint64_t);
    Span microseconds(std::uint64_t);
    Span nanoseconds(std::uint64_t);

  } // time
} // klee

#endif /* KLEE_TIME_H */
