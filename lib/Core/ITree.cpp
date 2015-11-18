/*
 * ITree.cpp
 *
 *  Created on: Oct 15, 2015
 *      Author: felicia
 */

#include "ITree.h"

#include <klee/Expr.h>
#include <vector>

using namespace klee;

void PathCondition::dump() {
	this->print(llvm::errs());
}

void PathCondition::print(llvm::raw_ostream & stream) {
	stream << "baseLoc = ";
	baseLoc->print(stream);
	stream << "\n";
	stream << "value = ";
	value->print(stream);
	stream << "\n";
	stream << "valueLoc = ";
	valueLoc->print(stream);
	stream << "\n";
	stream << "operationName = " << operationName << "\n";
	stream << "isRemoved = " << isRemoved << "\n";
}

ITree::ITree(const data_type &_root) : root(new ITreeNode(0,_root)) {
}

ITree::~ITree() {}

std::pair<ITreeNode*, ITreeNode*>
ITree::split(ITreeNode *n,
		 	 const data_type &leftData,
		     const data_type &rightData) {
  assert(n && !n->left && !n->right);
  n->left = new ITreeNode(n, leftData);
  n->right = new ITreeNode(n, rightData);
  return std::make_pair(n->left, n->right);
}

void ITree::addCondition(INode *n, ref<Expr> cond) {
  assert(!n->left && !n->right);
  do {
	INode *p = n->parent;
    if (p) {
      if (n == p->left) {
        p->left->conditions.push_back(cond);
      } else {
        assert(n == p->right);
        p->right->conditions.push_back(cond);
      }
    }
    n = p;
  } while (n && !n->left && !n->right);
}

ITreeNode::ITreeNode(ITreeNode *_parent,
        				ExecutionState *_data)
	: programPoint(0),
    parent(_parent),
    left(0),
    right(0),
    data(_data),
    interpolant(NULL),
    InterpolantStatus(NoInterpolant),
    isSubsumed(false){

	for (ConstraintManager::constraints_ty::const_iterator it = _data->constraints.begin(),
			ie = _data->constraints.end(); it != ie; ++it) {
		//	conditions = _data->constraints.getConstraints();
		conditions.push_back(*it);
	}
}

ITreeNode::~ITreeNode() {
}

void ITreeNode::dump() {
	this->print(llvm::errs());
}

void ITreeNode::print(llvm::raw_ostream &stream) {
	ITreeNode::print(stream, 0);
}

void ITreeNode::print(llvm::raw_ostream &stream, const unsigned int tab_num) {
	std::string tabs = make_tabs(tab_num);
	std::string tabs_next = tabs + "\t";

	stream << tabs << "ITreeNode\n";
	stream << tabs_next << "programPoint = " << (*programPoint) << "\n";
	stream << tabs_next << "data =\n";
	data->dumpStack(stream);

	stream << tabs_next << "conditions =";
	for (std::vector< ref<Expr> >::iterator it = conditions.begin(); it != conditions.end(); (stream << ","), it++) {
		((*it).get())->print(stream);
	}
	stream << "\n";

	stream << tabs_next << "Left:\n";
	left->print(stream, tab_num + 1);
	stream << tabs_next << "Right:\n";
	right->print(stream, tab_num + 1);

	stream << tabs_next << "newPathConds =";
	for (std::vector<PathCondition>::iterator it = newPathConds.begin(); it != newPathConds.end(); (stream << ","), it++) {
		it->print(stream);
	}
	stream << "\n";

	stream << tabs_next << "allPathConds =";
	for (std::vector<PathCondition>::iterator it = allPathConds.begin(); it != allPathConds.end(); (stream << ","), it++) {
		it->print(stream);
	}
	stream << "\n";

	stream << tabs_next << "dependenciesLoc =";
	for (std::vector< ref<Expr> >::iterator it = dependenciesLoc.begin(); it != dependenciesLoc.end(); (stream << ","), it++) {
		((*it).get())->print(stream);
	}
	stream << "\n";

	stream << tabs_next << "interpolant =";
	interpolant->print(stream);
	stream << tabs_next << "\n";

	stream << tabs_next << "InterpolantStatus =" << InterpolantStatus << "\n";

}



