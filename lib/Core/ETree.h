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
     * stores parent, left, right and some state data in each node. 
     * ETreeNodePtr is std::shared_ptr<ETreeNode>
    */
    class ETreeNode {
        public:

        ETreeNode *parent = nullptr;
        ETreeNodePtr left = nullptr;
        ETreeNodePtr right = nullptr;
        
        ProbExecState *state = nullptr;

        ETreeNode() = delete;

        ETreeNode(const ETreeNode &) {}
        ETreeNode(ETreeNode &&) {}
        ~ETreeNode() = default;

        explicit ETreeNode(ETreeNode* parent);
        ETreeNode(ETreeNode* parent, ProbExecState* state);
        ETreeNode(ETreeNode* parent, ProbExecState* state, ETreeNode* left, ETreeNode* right);
    };

    /**
     * Class for ETree definition. Execution Tree.
     * Objective is to print the ETree dump when executing a testcase. 
    */
    class ETree {
        public:
        ETreeNodePtr root = nullptr;
        ETreeNodePtr current = nullptr;
        
        ETree() = delete;
        explicit ETree(ProbExecState *state);
        ~ETree() = default;

        void forkState(ETreeNode* parentNode, bool flag, ProbExecState* leftState, ProbExecState* rightState);
        void removeNode(ETreeNode* delNode);
        void dumpETree(llvm::raw_ostream &fileptr);
        // void deleteNodes();
    };

} // namespace klee
#endif /* KLEE_ETREE_H */