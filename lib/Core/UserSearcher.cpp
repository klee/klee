//===-- UserSearcher.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Common.h"

#include "UserSearcher.h"

#include "Searcher.h"
#include "Executor.h"

#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace klee;

namespace {
  cl::opt<bool>
  UseRandomSearch("use-random-search");

  cl::opt<bool>
  UseInterleavedRS("use-interleaved-RS");

  cl::opt<bool>
  UseInterleavedNURS("use-interleaved-NURS");

  cl::opt<bool>
  UseInterleavedMD2UNURS("use-interleaved-MD2U-NURS");

  cl::opt<bool>
  UseInterleavedInstCountNURS("use-interleaved-icnt-NURS");

  cl::opt<bool>
  UseInterleavedCPInstCountNURS("use-interleaved-cpicnt-NURS");

  cl::opt<bool>
  UseInterleavedQueryCostNURS("use-interleaved-query-cost-NURS");

  cl::opt<bool>
  UseInterleavedCovNewNURS("use-interleaved-covnew-NURS");

  cl::opt<bool>
  UseNonUniformRandomSearch("use-non-uniform-random-search");

  cl::opt<bool>
  UseRandomPathSearch("use-random-path");

  cl::opt<WeightedRandomSearcher::WeightType>
  WeightType("weight-type", cl::desc("Set the weight type for --use-non-uniform-random-search"),
             cl::values(clEnumValN(WeightedRandomSearcher::Depth, "none", "use (2^depth)"),
                        clEnumValN(WeightedRandomSearcher::InstCount, "icnt", "use current pc exec count"),
                        clEnumValN(WeightedRandomSearcher::CPInstCount, "cpicnt", "use current pc exec count"),
                        clEnumValN(WeightedRandomSearcher::QueryCost, "query-cost", "use query cost"),
                        clEnumValN(WeightedRandomSearcher::MinDistToUncovered, "md2u", "use min dist to uncovered"),
                        clEnumValN(WeightedRandomSearcher::CoveringNew, "covnew", "use min dist to uncovered + coveringNew flag"),
                        clEnumValEnd));
  
  cl::opt<bool>
  UseMerge("use-merge", 
           cl::desc("Enable support for klee_merge() (experimental)"));
 
  cl::opt<bool>
  UseBumpMerge("use-bump-merge", 
           cl::desc("Enable support for klee_merge() (extra experimental)"));
 
  cl::opt<bool>
  UseIterativeDeepeningTimeSearch("use-iterative-deepening-time-search", 
                                    cl::desc("(experimental)"));

  cl::opt<bool>
  UseBatchingSearch("use-batching-search", 
           cl::desc("Use batching searcher (keep running selected state for N instructions/time, see --batch-instructions and --batch-time"));

  cl::opt<unsigned>
  BatchInstructions("batch-instructions",
                    cl::desc("Number of instructions to batch when using --use-batching-search"),
                    cl::init(10000));
  
  cl::opt<double>
  BatchTime("batch-time",
            cl::desc("Amount of time to batch when using --use-batching-search"),
            cl::init(5.0));
}

bool klee::userSearcherRequiresMD2U() {
  return (WeightType==WeightedRandomSearcher::MinDistToUncovered ||
          WeightType==WeightedRandomSearcher::CoveringNew ||
          UseInterleavedMD2UNURS ||
          UseInterleavedCovNewNURS || 
          UseInterleavedInstCountNURS || 
          UseInterleavedCPInstCountNURS || 
          UseInterleavedQueryCostNURS);
}

// FIXME: Remove.
bool klee::userSearcherRequiresBranchSequences() {
  return false;
}

Searcher *klee::constructUserSearcher(Executor &executor) {
  Searcher *searcher = 0;

  if (UseRandomPathSearch) {
    searcher = new RandomPathSearcher(executor);
  } else if (UseNonUniformRandomSearch) {
    searcher = new WeightedRandomSearcher(executor, WeightType);
  } else if (UseRandomSearch) {
    searcher = new RandomSearcher();
  } else {
    searcher = new DFSSearcher();
  }

  if (UseInterleavedNURS || UseInterleavedMD2UNURS || UseInterleavedRS ||
      UseInterleavedCovNewNURS || UseInterleavedInstCountNURS ||
      UseInterleavedCPInstCountNURS || UseInterleavedQueryCostNURS) {
    std::vector<Searcher *> s;
    s.push_back(searcher);
    
    if (UseInterleavedNURS)
      s.push_back(new WeightedRandomSearcher(executor, 
                                             WeightedRandomSearcher::Depth));
    if (UseInterleavedMD2UNURS)
      s.push_back(new WeightedRandomSearcher(executor, 
                                             WeightedRandomSearcher::MinDistToUncovered));

    if (UseInterleavedCovNewNURS)
      s.push_back(new WeightedRandomSearcher(executor, 
                                             WeightedRandomSearcher::CoveringNew));
   
    if (UseInterleavedInstCountNURS)
      s.push_back(new WeightedRandomSearcher(executor, 
                                             WeightedRandomSearcher::InstCount));
    
    if (UseInterleavedCPInstCountNURS)
      s.push_back(new WeightedRandomSearcher(executor, 
                                             WeightedRandomSearcher::CPInstCount));
    
    if (UseInterleavedQueryCostNURS)
      s.push_back(new WeightedRandomSearcher(executor, 
                                             WeightedRandomSearcher::QueryCost));

    if (UseInterleavedRS) 
      s.push_back(new RandomSearcher());

    searcher = new InterleavedSearcher(s);
  }

  if (UseBatchingSearch) {
    searcher = new BatchingSearcher(searcher, BatchTime, BatchInstructions);
  }

  if (UseMerge) {
    assert(!UseBumpMerge);
    searcher = new MergingSearcher(executor, searcher);
  } else if (UseBumpMerge) {    
    searcher = new BumpMergingSearcher(executor, searcher);
  }
  
  if (UseIterativeDeepeningTimeSearch) {
    searcher = new IterativeDeepeningTimeSearcher(searcher);
  }

  std::ostream &os = executor.getHandler().getInfoStream();

  os << "BEGIN searcher description\n";
  searcher->printName(os);
  os << "END searcher description\n";

  return searcher;
}
