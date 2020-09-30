#include "ETree.h"

#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Support/OptionCategories.h"
#include <vector>

using namespace klee;
using namespace llvm;

ETreeNode::ETreeNode(ETreeNode* parent)
    : parent{parent} {

    }

ETreeNode::ETreeNode(ETreeNode* parent, State state) : 
    parent{parent}, 
    state{&state} {
        left = ETreeNodePtr(nullptr);
        right = ETreeNodePtr(nullptr);
    } 

ETreeNode::ETreeNode(ETreeNode* parent, State state, ETreeNode *left, ETreeNode *right) :
    parent{parent}, 
    left{std::make_unique<ETreeNode>(left)}, 
    right{std::make_unique<ETreeNode>(right)}, 
    state{&state} {

}

ETree::ETree(State state) :
    root{std::make_unique<ETreeNode>(new ETreeNode(nullptr, state))} {

    }

void ETree::forkState(ETreeNode *parentNode, State *leftState, State *rightState) {
    // Fork the state, create a left and right side execution nodes. 
} 
        
void ETree::removeNode(ETreeNode *delNode) {
    // Remove a ETreeNode from the ETree
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



    delete pp;
}