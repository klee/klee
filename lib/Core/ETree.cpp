#include "ETree.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include <vector>

using namespace klee;
using namespace llvm;

ETreeNode::ETreeNode(ETreeNodePtr parent)
    : parent(parent) {

    }

ETreeNode::ETreeNode(ProbStatePtr state)
    : state(state) {

    }

ETreeNode::ETreeNode(ETreeNodePtr parent, ProbStatePtr state) : 
    parent(parent), 
    state(state) {

    }

ETreeNode::ETreeNode(ETreeNodePtr parent, ProbStatePtr state, ETreeNodePtr left, ETreeNodePtr right) :
    parent(parent), 
    left(left), 
    right(right), 
    state(state) {

}

ETree::ETree(ProbStatePtr initState) {
        root = std::make_shared<ETreeNode>(initState);
        current = root;
        root->left = nullptr;
        root->right = nullptr;
    }

void ETree::forkState(ETreeNodePtr Node, bool forkflag, ProbStatePtr leftState, ProbStatePtr rightState) {
    // Fork the state, create a left and right side execution nodes. 
    assert(Node.get() && !(Node.get()->left.get()) && !(Node.get()->right.get()));

    Node.get()->state.get()->data = Node.get()->state.get()->data == "Start" ? "Start" : "Forked";

    Node.get()->left = std::make_shared<ETreeNode>(Node, leftState);
    Node.get()->right = std::make_shared<ETreeNode>(Node, rightState);

    if (!leftState.get()->treeNode) {
        leftState.get()->treeNode = Node.get()->left;
    }

    if (!rightState.get()->treeNode) {
        rightState.get()->treeNode = Node.get()->right;
    }

    // We took a same decision as the PTree.cpp implementation.
    // Based on the Solver result. (trueNode or falseNode)
    this->current = forkflag ? Node.get()->left : Node.get()->right;
}

void ETree::removeNode(ETreeNodePtr delNode) {
    // // Remove a ETreeNode from the ETree
    if (!(delNode.get())) return;
    assert(delNode.get() && !(delNode.get()->right.get()) && !(delNode.get()->left.get()));
    
    do {
        // Must update the parent node accordingly. 
        ETreeNodePtr temp = delNode->parent;
        if (temp.get()) {
            if (delNode == temp.get()->left) {
                // We are on the left side.
                temp.get()->left.reset();
                temp.get()->left = nullptr;
            } else {
                // null it if the assert for right check passes. 
                assert(delNode == temp.get()->right);
                temp.get()->right.reset();
                temp.get()->right = nullptr;
            }
        }

        delNode.reset();
        delNode = temp;
    } while (delNode.get() && !(delNode.get()->right.get()) && !(delNode.get()->left.get()));
}
        
void ETree::dumpETree(llvm::raw_ostream &fileptr) {
    // Print the ETree to a dot file. 
    ExprPPrinter *pp = ExprPPrinter::create(fileptr);
    pp->setNewline("\\l");
    fileptr << "digraph G {\n";
    fileptr << "\tsize=\"10,5.5\";\n";
    fileptr << "\tratio=fill;\n";
    fileptr << "\trotate=90;\n";
    fileptr << "\tcenter = \"true\";\n";
    fileptr << "\tnode [width=.1, height=.1, fontname=\"Terminus\"]\n";

    std::vector<const ETreeNode*> processing_stack;
    processing_stack.emplace_back(root.get());
    std::string color = "black";

    // Process the stack, inorder on the tree. 
    while (!processing_stack.empty()) {
        const ETreeNode *current = processing_stack.back();
        processing_stack.pop_back();
        fileptr << "\t"; 
        current->state.get() ? 
                fileptr << "\"" << current->state.get()->stateId << ", " << current->state.get()->data << "\"" 
            :   fileptr << "no_state";
        fileptr << " [shape=ellipse, color=" << color << "];\n";
        
        // If node has left, process left. 
        if (current->left.get()) {
            fileptr << "\t" << "\"" << current->state.get()->stateId << ", " << current->state.get()->data << "\"" << " -> ";
            fileptr << "\"" << current->left.get()->state.get()->stateId;
            fileptr << ", ";
            fileptr << current->left.get()->state.get()->data << "\"";
            fileptr << " [label=L, color=green];\n";
            processing_stack.emplace_back(current->left.get());
        }

        // If node has right side, process right side. 
        if (current->right.get()) {
            fileptr << "\t" << "\"" << current->state.get()->stateId << ", " << current->state.get()->data << "\"" << " -> ";
            fileptr << "\"" << current->right.get()->state.get()->stateId;
            fileptr << ", ";
            fileptr << current->right.get()->state.get()->data << "\"";
            fileptr << " [label=R, color=red];\n";
            processing_stack.emplace_back(current->right.get());
        }
    }

    fileptr << "}\n";
    delete pp;
}