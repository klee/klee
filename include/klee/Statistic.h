//===-- Statistic.h ---------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_STATISTIC_H
#define KLEE_STATISTIC_H

#include <string>

namespace klee {
  class Statistic;
  class StatisticManager;
  class StatisticRecord;

  /// Statistic - A named statistic instance.
  ///
  /// The Statistic class holds information about the statistic, but
  /// not the actual values. Values are managed by the global
  /// StatisticManager to enable transparent support for instruction
  /// level and call path level statistics.
  class Statistic final {
    friend class StatisticManager;
    friend class StatisticRecord;

  private:
    std::uint32_t id;
    const std::string name;
    const std::string shortName;

  public:
    Statistic(const std::string &name, const std::string &shortName);
    ~Statistic() = default;

    /// getID - Get the unique statistic ID.
    std::uint32_t getID() const { return id; }

    /// getName - Get the statistic name.
    const std::string &getName() const { return name; }

    /// getShortName - Get the "short" statistic name, used in
    /// callgrind output for example.
    const std::string &getShortName() const { return shortName; }

    /// getValue - Get the current primary statistic value.
    std::uint64_t getValue() const;

    /// operator std::uint64_t - Get the current primary statistic value.
    operator std::uint64_t() const { return getValue(); }

    /// operator++ - Increment the statistic by 1.
    Statistic &operator++() { return (*this += 1); }

    /// operator+= - Increment the statistic by \arg addend.
    Statistic &operator+=(std::uint64_t addend);
  };
}

#endif /* KLEE_STATISTIC_H */
