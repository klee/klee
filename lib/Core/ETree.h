#ifndef KLEE_ETREE_H
#define KLEE_ETREE_H

#include "klee/Expr/Expr.h"

#include <memory>
#include <string>
#include <unordered_map>

/**
 * This is a custom tree data structure to store some information during
 * execution Later it can be extended to store path-specific execution state
 * related information.
 */
namespace klee {

    class ETreeNode;
    class Etree;
    class State;

    using ETreeNodePtr = std::unique_ptr<ETreeNode>;
    typedef unsigned long long int BigInteger;

    /**
     * Store some custom State data in the 
     * tree ndoes for ETree.
    */
    class State {
        public:
        std::string data;
        BigInteger id;

        State() = delete;

        State(const State &) {}
        State(State &&) {}
        ~State() = default;

        State &operator=(const State &) { return *this; }
        State &operator=(State &&) { return *this; }

        State(std::string data, BigInteger id);
    };

    /**
     * ETreeNode Class
     * tree ndoes for ETree.
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

        ETreeNode &operator=(const ETreeNode &) { return *this; }
        ETreeNode &operator=(ETreeNode &&) { return *this; }

        explicit ETreeNode(State state);
        ETreeNode(State state, ETreeNode *left, ETreeNode *right);
    };

    /**
     * Objective is to print the ETree dump when executing a testcase. 
    */
    class ETree {
        public:
        ETreeNodePtr root;
        explicit ETree(State state);
        ~ETree() = default;

        void forkState(ETreeNode *parentNode, State *leftState, State *rightState);
        void removeNode(ETreeNode *delNode);
        void dumpETree(llvm::raw_ostream &fileptr);
    };

} // namespace klee
#endif /* KLEE_ETREE_H */