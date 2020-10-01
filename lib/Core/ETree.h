#ifndef KLEE_ETREE_H
#define KLEE_ETREE_H

#include "klee/Expr/Expr.h"

#include <memory>
#include <string>
#include <unordered_map>

/**
 * This is a custom execution Tree to store some information during
 * execution. Later it can be extended to store path-specific execution state
 * related information like the PTree implementation. 
 */
namespace klee {

    class ETreeNode;
    class Etree;
    class State;

    using ETreeNodePtr = std::shared_ptr<ETreeNode>;
    using ETreeNodePtrUnique = std::unique_ptr<ETreeNode>;
    typedef unsigned long long int BigInteger;

    /**
     * Store some custom State data in the 
     * tree ndoes for ETree. Stores data & id.
     * BigInteger is ULL. 
    */
    class State {
        public:

        // Data Store
        std::string data;
        BigInteger id;
        ETreeNodePtrUnique associatedTreeNode;
        
        State() = delete;

        State(const State &) {}
        State(State &&) {}
        ~State() = default;

        // State &operator=(const State &) { return *this; }
        // State &operator=(State &&) { return *this; }
        
        State(std::string data, BigInteger id);
        State(std::string data, BigInteger id, ETreeNode* current);
    };

    /**
     * Class for nodes of ETree.
     * stores parent, left, right and some state data in each node. 
     * ETreeNodePtr is std::shared_ptr<ETreeNode>
    */
    class ETreeNode {
        public:
        ETreeNode *parent = nullptr;

        ETreeNodePtr left;
        ETreeNodePtr right;
        State *state = nullptr;

        ETreeNode() = delete;

        ETreeNode(const ETreeNode &) {}
        ETreeNode(ETreeNode &&) {}
        ~ETreeNode() = default;

        // ETreeNode &operator=(const ETreeNode &) { return *this; }
        // ETreeNode &operator=(ETreeNode &&) { return *this; }

        explicit ETreeNode(ETreeNode* parent);
        ETreeNode(ETreeNode* parent, State* state);
        ETreeNode(ETreeNode* parent, State* state, ETreeNode* left, ETreeNode* right);
    };

    /**
     * Class for ETree definition. Execution Tree.
     * Objective is to print the ETree dump when executing a testcase. 
    */
    class ETree {
        public:
        ETreeNodePtr root;
        ETreeNodePtr current;
        
        ETree() = delete;
        explicit ETree(State *state);
        ~ETree() = default;

        void forkState(ETreeNode* parentNode, State* leftState, State* rightState);
        void removeNode(ETreeNode* delNode);
        void dumpETree(llvm::raw_ostream &fileptr);
    };

} // namespace klee
#endif /* KLEE_ETREE_H */