#include "ETree.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Support/OptionCategories.h"
#include <vector>

using namespace klee;
using namespace llvm;

State::State(std::string data, BigInteger id, ETreeNode *current) :
    data{data},
    id{id},
    associatedTreeNode{ETreeNodePtrUnique(current)} {
     // Create a new state data entry
}

State::State(std::string data, BigInteger id) :
    data{data},
    id{id},
    associatedTreeNode{nullptr} {
     // Create a new state data entry
}

ETreeNode::ETreeNode(ETreeNode* parent)
    : parent{parent} {
        this->left = nullptr;
        this->right = nullptr;
        this->state = nullptr;
    }

ETreeNode::ETreeNode(ETreeNode* parent, State *state) : 
    parent{parent}, 
    state{state} {
        this->left = nullptr;
        this->right = nullptr;
    }

// No Forking, this adds extra redundent nodes. 
ETreeNode::ETreeNode(ETreeNode* parent, State *state, ETreeNode *left, ETreeNode *right) :
    parent{parent}, 
    left{ETreeNodePtr(left)}, 
    right{ETreeNodePtr(right)}, 
    state{state} {

}

ETree::ETree(State *initState) {
        ETreeNode* rootNode = new ETreeNode(nullptr, initState);
        // initState->execTreeNode = ETreeNodePtrUnique(rootNode);
        root = ETreeNodePtr(rootNode);
        current = root;
        root->left = nullptr;
        root->right = nullptr;
    }

// FIXME : Assert fails. Bug in Fork or add state.
// TODO : Assign the current nodes correctly to the left and right states. 
void ETree::forkState(ETreeNode *Node, int flag, State *leftState, State *rightState) {
    // Fork the state, create a left and right side execution nodes. 
    assert(Node && !(Node->left.get()) && !(Node->right.get()));
    ETreeNode* tempLeft = new ETreeNode(Node, leftState);
    ETreeNode* tempRight = new ETreeNode(Node, rightState);

    Node->state->data = "Forked";
    Node->left = ETreeNodePtr(tempLeft);
    Node->right = ETreeNodePtr(tempRight);

    if (leftState->associatedTreeNode) {
        leftState->associatedTreeNode = ETreeNodePtrUnique(tempLeft);
    }

    if (rightState->associatedTreeNode) {
        rightState->associatedTreeNode = ETreeNodePtrUnique(tempRight);
    }

    // FIXME : Update this correctly. 
    // We took a same decision as the PTree.cpp implementation.
    this->current = flag ? Node->left : Node->right;
} 
        
// FIXME : Assert fails. Bug in Fork or add state.
// TODO : Assign the current nodes correctly to states on removal. 
void ETree::removeNode(ETreeNode *delNode) {
    // Remove a ETreeNode from the ETree
    assert(delNode && !delNode->right.get() && !delNode->left.get());
    do {
        // Must update the parent node accordingly. 
        ETreeNode *temp = delNode->parent;
        if (temp) {
            if (delNode == temp->left.get()) {
                // We are on the left side.
                temp->left = nullptr;
            } else {
                // null it if the assert for right check passes. 
                assert(delNode == temp->right.get());
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
    fileptr << "\tnode [style=\"filled\",width=.1,height=.1,fontname=\"Terminus\"]\n";

    std::vector<const ETreeNode*> processing_stack;
    processing_stack.emplace_back(root.get());

    // Process the stack, inorder on the tree. 
    while (!processing_stack.empty()) {
        const ETreeNode *current = processing_stack.back();
        processing_stack.pop_back();
        fileptr << "\t"; 
        current->state ? 
                fileptr << "\"" << current->state->id << "," << current->state->data << "\"" 
            :   fileptr << "__no_state__";
        fileptr << " [shape=diamond, fillcolor=green];\n";
        
        if (current->left.get()) {
            fileptr << "\t" << "\"" << current->state->id << "," << current->state->data << "\"" << " -> ";
            fileptr << "\"" << current->left.get()->state->id;
            fileptr << ",";
            fileptr << current->left.get()->state->data << "\"";
            fileptr << " [label=L, color=green];\n";
            processing_stack.emplace_back(current->left.get());
        }

        if (current->right.get()) {
            fileptr << "\t" << "\"" << current->state->id << "," << current->state->data << "\"" << " -> ";
            fileptr << "\"" << current->right.get()->state->id;
            fileptr << ",";
            fileptr << current->right.get()->state->data << "\"";
            fileptr << " [label=R, color=red];\n";
            processing_stack.emplace_back(current->right.get());
        }
    }

    fileptr << "}\n";
    delete pp;
}