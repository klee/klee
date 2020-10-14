#include "Executor.h"
#include "Searcher.h"

#define DEBUG_TYPE "zesti"
using namespace klee;
using namespace llvm;

ZESTIPendingSearcher::ZESTIPendingSearcher(Executor &_exec, int _zestiBound) : exec(_exec), ZestiBound(_zestiBound) {
  normalSearcher = std::make_unique<DFSSearcher>();
  bound = 0;
  // Register a callback that records the depth of all loads/stores with symbolic pointer.
  exec.memoryCallback = [&](auto &es, auto check) {
    if (!isa<ConstantExpr>(check))
      senstiveDepths.insert(es.depth);
  };
}

ExecutionState &ZESTIPendingSearcher::selectState() {
  if (!hasSelectedState) {
    computeDistances();
    // Doing zesti exploration now, stop recording sensitive depths
    exec.memoryCallback =
        std::function<void(const ExecutionState &, ref<Expr>)>();
  }
  hasSelectedState = true;
  for (auto &es : toDelete) {
    exec.terminateState(*es);
  }
  toDelete.clear();

  return normalSearcher->selectState();
}

void ZESTIPendingSearcher::update(
    ExecutionState *current, const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {
  auto is_pending = [](const auto &es) {
    return !es->pendingConstraint.isNull();
  };
  std::vector<ExecutionState *> addedN, addedP, removedN, removedP;

  for (const auto &es : addedStates) {
    if (is_pending(es))
      addedP.push_back(es);
    else if (currentBaseDepth >= 0 &&
             (int)es->depth > currentBaseDepth + bound) {
      // This a normal state, out of bounds of current exploration, 
      // therefore we termiante it
      KLEE_DEBUG(
        llvm::errs() << "Terminating added state due to its depth: " << es->depth 
                     << " exceeding base depth " << currentBaseDepth 
                     << " + " << bound << "\n";
      );
      toDelete.push_back(es);
    } else
      addedN.push_back(es);
  }

  for (const auto &es : removedStates) {
    if (is_pending(es))
      removedP.push_back(es);
    else
      removedN.push_back(es);
  }

  if (current && is_pending(current)) {
    removedN.push_back(current);
    addedP.push_back(current);
  } else if (current && currentBaseDepth >= 0 &&
             (int)current->depth > currentBaseDepth + bound) {
    // This a normal state, out of bounds of current exploration, 
    // therefore we termiante it
    KLEE_DEBUG(
      llvm::errs() << "Terminating current state due to its depth: " << current->depth 
                   << " exceeding base depth " << currentBaseDepth 
                   << " + " << bound << "\n";
    );
    bool currentIsRemoved = false;
    for (const auto &es : removedStates) {
      currentIsRemoved |= es == current;
    }

    if (!currentIsRemoved)
      toDelete.push_back(current);
    current = nullptr;
  }

  if (hasSelectedState) {
    assert(addedP.empty() && "ZESTI searcher assumes pending states has been "
                             "disabled when it is active");
  }
  normalSearcher->update(current, addedN, removedN);

  pendingStates.insert(pendingStates.end(), addedP.begin(), addedP.end());
  for (const auto &es : removedP) {
    auto it = std::find(pendingStates.begin(), pendingStates.end(), es);
    if(it != pendingStates.end()) pendingStates.erase(it);
  }
}

void ZESTIPendingSearcher::computeDistances() {
 
  for (const auto &pEs : pendingStates) {
    int smallestDistance = 999999;
    for (const auto &sDepth : senstiveDepths) {
      int diff = sDepth - pEs->depth;
      smallestDistance =
          diff >= 0 && diff < smallestDistance ? diff : smallestDistance;
    }
    smallestSensitiveDistance[pEs] = smallestDistance;
  }

  std::sort(pendingStates.begin(), pendingStates.end(),
            [&](const ExecutionState *es1, const ExecutionState *es2) {
              if (smallestSensitiveDistance[es1] ==
                  smallestSensitiveDistance[es2])
                return es1->depth < es2->depth;
              else
                return (smallestSensitiveDistance[es1] >
                        smallestSensitiveDistance[es2]);
            });
}

bool ZESTIPendingSearcher::empty() {
  if (!hasSelectedState) {
    computeDistances();
  }
  hasSelectedState = true;
  // If zesti bound is 0, we do no exploration of pending states 
  // (ie. simple zesti)
  if (ZestiBound == 0) {
    return true;
  }
  // If there are no normal states we need to revive one
  while (normalSearcher->empty() && !pendingStates.empty()) {

    KLEE_DEBUG(
      llvm::errs() << senstiveDepths.size()
                   << " Sensitive instructions at depths: [";
      for (const auto &dpth : senstiveDepths) {
        llvm::errs() << dpth << ", ";
      }
      llvm::errs() << "\b\b]\n";

      llvm::errs() << "Pending ordering depths: [";
      for (const auto &pEs : pendingStates) {
        llvm::errs() << pEs->depth << ", ";
      }
      llvm::errs() << "\b\b]\n";
    );

    // Get the next pending state
    auto es = pendingStates.back();
    pendingStates.pop_back();

    //If this state is not past the last sensitive instruction try to revive it
    if (smallestSensitiveDistance[es] != 999999 &&
        exec.attemptToRevive(es, exec.solver->solver.get())) {

      currentBaseDepth = es->depth;
      bound = ZestiBound * smallestSensitiveDistance[es];
      update(nullptr, {es}, {});
      KLEE_DEBUG(
        llvm::errs() << "ZESTI revive state at depth " << es->depth
                     << " adding bound " << bound << "\n";
      );
    } else {
      KLEE_DEBUG(llvm::errs() << "ZESTI kill state at depth " << es->depth << "\n";);
      exec.terminateState(*es);
    }
  }
  return normalSearcher->empty();
}
