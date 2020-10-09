#include "ETree.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include <vector>

using namespace klee;
using namespace llvm;

ETreeNode::ETreeNode(ETreeNode* parent)
    : parent{parent} {
        this->left = nullptr;
        this->right = nullptr;
        this->state = nullptr;
    }

ETreeNode::ETreeNode(ETreeNode* parent, ProbExecState* state) : 
    parent{parent}, 
    state{state} {
        this->left = nullptr;
        this->right = nullptr;
    }

// No Forking, this adds extra redundent nodes. 
ETreeNode::ETreeNode(ETreeNode* parent, ProbExecState* state, ETreeNode* left, ETreeNode* right) :
    parent{parent}, 
    left{ETreeNodePtr(left)}, 
    right{ETreeNodePtr(right)}, 
    state{state} {

}

ETree::ETree(ProbExecState *initState) {
        ETreeNode* rootNode = new ETreeNode(nullptr, initState);
        root = ETreeNodePtr(rootNode);
        current = root;
        root->left = nullptr;
        root->right = nullptr;
    }

void ETree::forkState(ETreeNode *Node, bool forkflag, ProbExecState *leftState, ProbExecState *rightState) {
    // Fork the state, create a left and right side execution nodes. 
    assert(Node && !(Node->left.get()) && !(Node->right.get()));
    ETreeNode* tempLeft = new ETreeNode(Node, leftState);
    ETreeNode* tempRight = new ETreeNode(Node, rightState);

    Node->state->data = Node->state->data == "Start" ? "Start" : "Forked";

    Node->left = ETreeNodePtr(tempLeft);
    Node->right = ETreeNodePtr(tempRight);

    if (!leftState->treeNode) {
        leftState->treeNode = (tempLeft);
    }

    if (!rightState->treeNode) {
        rightState->treeNode = (tempRight);
    }

    // We took a same decision as the PTree.cpp implementation.
    // Based on the Solver result. (trueNode or falseNode)
    this->current = forkflag ? Node->left : Node->right;
}

void ETree::removeNode(ETreeNode *delNode) {
    // Remove a ETreeNode from the ETree
    if (!delNode) return;
    // assert(delNode && !(delNode->right.get()) && !(delNode->left.get()));
    do {
        // Must update the parent node accordingly. 
        ETreeNode *temp = delNode->parent;
        if (temp) {
            if (delNode == temp->left.get()) {
                // We are on the left side.
                temp->left.reset();
                temp->left = nullptr;
            } else {
                // null it if the assert for right check passes. 
                assert(delNode == temp->right.get());
                temp->right.reset();
                temp->right = nullptr;
            }
        }
        delete delNode;
        delNode = temp;
    } while (delNode && !delNode->right.get() && !delNode->left.get());
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
        current->state ? 
                fileptr << "\"" << current->state->stateId << ", " << current->state->data << "\"" 
            :   fileptr << "no_state";
        fileptr << " [shape=ellipse, color=" << color << "];\n";
        
        // If node has left, process left. 
        if (current->left.get()) {
            fileptr << "\t" << "\"" << current->state->stateId << ", " << current->state->data << "\"" << " -> ";
            fileptr << "\"" << current->left.get()->state->stateId;
            fileptr << ", ";
            fileptr << current->left.get()->state->data << "\"";
            fileptr << " [label=L, color=green];\n";
            processing_stack.emplace_back(current->left.get());
        }

        // If node has right side, process right side. 
        if (current->right.get()) {
            fileptr << "\t" << "\"" << current->state->stateId << ", " << current->state->data << "\"" << " -> ";
            fileptr << "\"" << current->right.get()->state->stateId;
            fileptr << ", ";
            fileptr << current->right.get()->state->data << "\"";
            fileptr << " [label=R, color=red];\n";
            processing_stack.emplace_back(current->right.get());
        }
    }

    fileptr << "}\n";
    delete pp;
}