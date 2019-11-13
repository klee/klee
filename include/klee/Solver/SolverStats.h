//===-- SolverStats.h -------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_SOLVERSTATS_H
#define KLEE_SOLVERSTATS_H

#include "klee/Statistic.h"

namespace klee {
namespace stats {

extern SQLIntStatistic cexCacheTime;
extern SQLIntStatistic queries;
extern SQLIntStatistic queriesInvalid;
extern SQLIntStatistic queriesValid;
extern SQLIntStatistic queryCacheHits;
extern SQLIntStatistic queryCacheMisses;
extern SQLIntStatistic queryCexCacheHits;
extern SQLIntStatistic queryCexCacheMisses;
extern SQLIntStatistic queryConstructTime;
extern SQLIntStatistic queryConstructs;
extern SQLIntStatistic queryCounterexamples;
extern SQLIntStatistic queryTime;

#ifdef KLEE_ARRAY_DEBUG
extern SQLIntStatistic arrayHashTime;
#endif

}
}

#endif /* KLEE_SOLVERSTATS_H */
