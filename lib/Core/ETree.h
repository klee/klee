#ifndef KLEE_ETREE_H
#define KLEE_ETREE_H

#include "ProbExecState.h"
#include "klee/Expr/Expr.h"

#include <memory>

/**
 * This is a custom execution Tree to store some information during
 * execution. Later it can be extended to store path-specific execution state
 * related information like the PTree implementation.
 */
namespace klee {

class Etree;

/**
 * Class for nodes of ETree.
 * stores parent, left, right and ProbStatePtr data in each node.
 * ETreeNodePtr is std::shared_ptr<ETreeNode>
 */
class ETreeNode {
public:
  ETreeNodePtr parent = nullptr;
  ETreeNodePtr left = nullptr;
  ETreeNodePtr right = nullptr;
  ProbStatePtr state = nullptr;

  ETreeNode() = delete;
  ETreeNode(const ETreeNode &) {}
  ETreeNode(ETreeNode &&) {}
  ~ETreeNode() = default;

  explicit ETreeNode(ETreeNodePtr parent);
  explicit ETreeNode(ProbStatePtr state);
  ETreeNode(ETreeNodePtr parent, ProbStatePtr state);
  ETreeNode(ETreeNodePtr parent, ProbStatePtr state, ETreeNodePtr left,
            ETreeNodePtr right);
};

/**
 * Class for ETree definition. Prob Execution Tree for forks and branches
 * Objective is to print the ETree dump when executing a testcase.
 */
class ETree {
public:
  ETreeNodePtr root = nullptr;
  ETreeNodePtr current = nullptr;

  ETree() = delete;
  explicit ETree(ProbStatePtr state);
  ~ETree() = default;

  void forkState(ETreeNodePtr parentNode, bool flag, ProbStatePtr leftState,
                 ProbStatePtr rightState);
  void removeNode(ETreeNodePtr delNode);
  void dumpETree(llvm::raw_ostream &fileptr);
};

} // namespace klee
#endif /* KLEE_ETREE_H */