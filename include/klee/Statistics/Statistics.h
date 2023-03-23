//===-- Statistics.h --------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_STATISTICS_H
#define KLEE_STATISTICS_H

#include "Statistic.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace klee {
class Statistic;
class StatisticRecord {
  friend class StatisticManager;

private:
  std::uint64_t *data;

public:
  StatisticRecord();
  StatisticRecord(const StatisticRecord &s);
  ~StatisticRecord() { delete[] data; }

  void zero();

  std::uint64_t getValue(const Statistic &s) const;
  void incrementValue(const Statistic &s, std::uint64_t addend) const;
  StatisticRecord &operator=(const StatisticRecord &s);
  StatisticRecord &operator+=(const StatisticRecord &sr);
};

class StatisticManager {
private:
  bool enabled;
  std::vector<Statistic *> stats;
  std::uint64_t *globalStats;
  std::uint64_t *indexedStats;
  StatisticRecord *contextStats;
  unsigned index;

public:
  StatisticManager();
  ~StatisticManager();

  void useIndexedStats(unsigned totalIndices);

  StatisticRecord *getContext();
  void setContext(StatisticRecord *sr); /* null to reset */

  void setIndex(unsigned i) { index = i; }
  unsigned getIndex() { return index; }
  unsigned getNumStatistics() { return stats.size(); }
  Statistic &getStatistic(unsigned i) { return *stats[i]; }

  void registerStatistic(Statistic &s);
  void incrementStatistic(Statistic &s, std::uint64_t addend);
  std::uint64_t getValue(const Statistic &s) const;
  void incrementIndexedValue(const Statistic &s, unsigned index,
                             std::uint64_t addend) const;
  std::uint64_t getIndexedValue(const Statistic &s, unsigned index) const;
  void setIndexedValue(const Statistic &s, unsigned index, std::uint64_t value);
  int getStatisticID(const std::string &name) const;
  Statistic *getStatisticByName(const std::string &name) const;
};

extern StatisticManager *theStatisticManager;

inline void StatisticManager::incrementStatistic(Statistic &s,
                                                 std::uint64_t addend) {
  if (enabled) {
    globalStats[s.id] += addend;
    if (indexedStats) {
      indexedStats[index * stats.size() + s.id] += addend;
      if (contextStats)
        contextStats->data[s.id] += addend;
    }
  }
}

inline StatisticRecord *StatisticManager::getContext() { return contextStats; }
inline void StatisticManager::setContext(StatisticRecord *sr) {
  contextStats = sr;
}

inline void StatisticRecord::zero() {
  std::memset(data, 0, sizeof(*data) * theStatisticManager->getNumStatistics());
}

inline StatisticRecord::StatisticRecord()
    : data(new std::uint64_t[theStatisticManager->getNumStatistics()]) {
  zero();
}

inline StatisticRecord::StatisticRecord(const StatisticRecord &s)
    : data(new std::uint64_t[theStatisticManager->getNumStatistics()]) {
  std::memcpy(data, s.data,
              sizeof(*data) * theStatisticManager->getNumStatistics());
}

inline StatisticRecord &StatisticRecord::operator=(const StatisticRecord &s) {
  std::memcpy(data, s.data,
              sizeof(*data) * theStatisticManager->getNumStatistics());
  return *this;
}

inline void StatisticRecord::incrementValue(const Statistic &s,
                                            std::uint64_t addend) const {
  data[s.id] += addend;
}
inline std::uint64_t StatisticRecord::getValue(const Statistic &s) const {
  return data[s.id];
}

inline StatisticRecord &StatisticRecord::operator+=(const StatisticRecord &sr) {
  unsigned nStats = theStatisticManager->getNumStatistics();
  for (unsigned i = 0; i < nStats; i++)
    data[i] += sr.data[i];
  return *this;
}

inline std::uint64_t StatisticManager::getValue(const Statistic &s) const {
  return globalStats[s.id];
}

inline void
StatisticManager::incrementIndexedValue(const Statistic &s, unsigned index,
                                        std::uint64_t addend) const {
  indexedStats[index * stats.size() + s.id] += addend;
}

inline std::uint64_t StatisticManager::getIndexedValue(const Statistic &s,
                                                       unsigned index) const {
  return indexedStats[index * stats.size() + s.id];
}

inline void StatisticManager::setIndexedValue(const Statistic &s,
                                              unsigned index,
                                              std::uint64_t value) {
  indexedStats[index * stats.size() + s.id] = value;
}
} // namespace klee

#endif /* KLEE_STATISTICS_H */
