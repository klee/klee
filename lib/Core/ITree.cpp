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
