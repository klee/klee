#include "ETree.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Support/OptionCategories.h"
#include <vector>

using namespace klee;
using namespace llvm;

State::State(std::string data, BigInteger id) :
    data{data},
    id{id} {
     // Create a new state data entry
}

ETreeNode::ETreeNode(ETreeNode* parent)
    : parent{parent} {

    }

ETreeNode::ETreeNode(ETreeNode* parent, State *state) : 
    parent{parent}, 
    state{state} {
        left = std::make_unique<ETreeNode>(nullptr);
        right = std::make_unique<ETreeNode>(nullptr);
    }

ETreeNode::ETreeNode(ETreeNode* parent, State *state, ETreeNode *left, ETreeNode *right) :
    parent{parent}, 
    left{std::make_unique<ETreeNode>(left)}, 
    right{std::make_unique<ETreeNode>(right)}, 
    state{state} {

}

ETree::ETree(State *state) :
    root{std::make_unique<ETreeNode>(new ETreeNode(nullptr, state))} {

    }

void ETree::forkState(ETreeNode *parentNode, State *leftState, State *rightState) {
    // Fork the state, create a left and right side execution nodes. 
    assert(parentNode && !parentNode->left.get() && !parentNode->right.get());
    parentNode->state->data = "Node Forked";
    parentNode->left = std::make_unique<ETreeNode>(new ETreeNode(nullptr, leftState));
    parentNode->right = std::make_unique<ETreeNode>(new ETreeNode(nullptr, rightState));
} 
        
void ETree::removeNode(ETreeNode *delNode) {
    // Remove a ETreeNode from the ETree
    assert(delNode && !delNode->right.get() && !delNode->left.get());
    do {
        // Must update the parent node accordingly. 
        ETreeNode *temp = delNode->parent;
        if (temp) {
            if (delNode == temp->left.get()) {
                // We are on the left side.
                temp->left = std::make_unique<ETreeNode>(nullptr);
            } else {
                // null it if the assert for right check passes. 
                assert(delNode == temp->right.get());
                temp->right = std::make_unique<ETreeNode>(nullptr);
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
    fileptr << "\tsize=\"10,7.5\";\n";
    fileptr << "\tratio=fill;\n";
    fileptr << "\trotate=90;\n";
    fileptr << "\tcenter = \"true\";\n";
    fileptr << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n";
    fileptr << "\tedge [arrowsize=.4]\n";

    std::vector<const ETreeNode*> processing_stack;
    processing_stack.push_back(root.get());

    // Process the stack, inorder on the tree. 
    while (!processing_stack.empty()) {
        const ETreeNode *current = processing_stack.back();
        processing_stack.pop_back();
        fileptr << "\tNode : "; 
        fileptr << current->state->id;
        fileptr << " [shape=ellipse, fillcolor=green];\n";
        
        if (current->left.get()) {
            fileptr << "\tNode : " << current->state->id << " -> Node " << current->left.get()->state->id;
            fileptr << "[label=Left ";
            fileptr << current->left.get()->state->data << "];\n";
            processing_stack.push_back(current->left.get());
        }

        if (current->right.get()) {
            fileptr << "\tNode : " << current->state->id << " -> Node " << current->right.get()->state->id;
            fileptr << "[label=Right ";
            fileptr << current->right.get()->state->data << "];\n";
            processing_stack.push_back(current->right.get());
        }
    }

    fileptr << "}\n";
    delete pp;
}