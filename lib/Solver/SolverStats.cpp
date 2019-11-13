//===-- SolverStats.cpp ---------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Solver/SolverStats.h"

using namespace klee;

SQLIntStatistic stats::cexCacheTime("CexCacheTime", "CCtime");
SQLIntStatistic stats::queries("Queries", "Q");
SQLIntStatistic stats::queriesInvalid("QueriesInvalid", "Qiv", 1);
SQLIntStatistic stats::queriesValid("QueriesValid", "Qv", 1);
SQLIntStatistic stats::queryCacheHits("QueryCacheHits", "QChits", 1);
SQLIntStatistic stats::queryCacheMisses("QueryCacheMisses", "QCmisses", 1);
SQLIntStatistic stats::queryCexCacheHits("QueryCexCacheHits", "QCexHits");
SQLIntStatistic stats::queryCexCacheMisses("QueryCexCacheMisses", "QCexMisses");
SQLIntStatistic stats::queryConstructTime("QueryConstructTime", "QBtime", 1);
SQLIntStatistic stats::queryConstructs("QueriesConstructs", "QB");
SQLIntStatistic stats::queryCounterexamples("QueriesCEX", "Qcex", 1);
SQLIntStatistic stats::queryTime("QueryTime", "Qtime");

#ifdef KLEE_ARRAY_DEBUG
SQLIntStatistic stats::arrayHashTime("ArrayHashTime", "AHtime");
#endif
