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

UpdateRelation::UpdateRelation(const ref<Expr>& baseLoc, const ref<Expr>& value, const Operation& operationName) :
    base(0), valueLoc(0) {
  this->baseLoc = baseLoc;
  this->value = value;
  this->operationName = operationName;
}

UpdateRelation::~UpdateRelation() {}

ref<Expr> UpdateRelation::makeExpr(ref<Expr>& locToCompare, ref<Expr>& lhs) const {
  if (baseLoc != locToCompare)
    return lhs;

  switch (operationName) {
    case Add:
      return AddExpr::create(lhs, value);
    case Sub:
      return SubExpr::create(lhs, value);
    case Mul:
      return MulExpr::create(lhs, value);
    case UDiv:
      return UDivExpr::create(lhs, value);
    case SDiv:
      return SDivExpr::create(lhs, value);
    case URem:
      return URemExpr::create(lhs, value);
    case SRem:
      return SRemExpr::create(lhs, value);
    case And:
      return AndExpr::create(lhs, value);
    case Or:
      return OrExpr::create(lhs, value);
    case Xor:
      return XorExpr::create(lhs, value);
    case Shl:
      return ShlExpr::create(lhs, value);
    case LShr:
      return LShrExpr::create(lhs, value);
    case AShr:
      return AShrExpr::create(lhs, value);
    default:
      return lhs;
  }
}

void UpdateRelation::setBase(const ref<Expr>& base) {
  this->base = base;
}

void UpdateRelation::setValueLoc(const ref<Expr>& valueLoc) {
  this->valueLoc = valueLoc;
}

ref<Expr> UpdateRelation::getBaseLoc() const {
  return baseLoc;
}

bool UpdateRelation::isBase(ref<Expr>& expr) const {
  return !base.isNull() && base == expr;
}

void UpdateRelation::dump() const {
  this->print(llvm::errs());
}

void UpdateRelation::print(llvm::raw_ostream & stream) const {
  stream << "base = ";
  if (!base.isNull()) {
      base->print(stream);
  } else {
      stream << "NULL";
  }
  stream << "\n";
  stream << "baseLoc = ";
  if (!baseLoc.isNull()) {
      baseLoc->print(stream);
  } else {
      stream << "NULL";
  }
  stream << "\n";
  stream << "value = ";
  if (!value.isNull()) {
      value->print(stream);
  } else {
      stream << "NULL";
  }
  stream << "\n";
  stream << "valueLoc = ";
  if (!valueLoc.isNull()) {
      valueLoc->print(stream);
  } else {
      stream << "NULL";
  }
  stream << "\n";
  stream << "operationName = " << operationName << "\n";
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
  llvm::errs() << "\n------------------------- Root ITree --------------------------------\n";
  this->print(llvm::errs());
}

void ITreeNode::print(llvm::raw_ostream &stream) {
  ITreeNode::print(stream, 0);
}

void ITreeNode::print(llvm::raw_ostream &stream, const unsigned int tab_num) {
  std::string tabs = make_tabs(tab_num);
  std::string tabs_next = tabs + "\t";

  stream << tabs << "ITreeNode\n";
  stream << tabs_next << "programPoint = ";
  if (!programPoint) {
      stream << "NULL";
  } else {
      stream << (*programPoint);
  }
  stream << "\n";
  stream << tabs_next << "conditions =";
  for (std::vector< ref<Expr> >::iterator it = conditions.begin(); it != conditions.end(); (stream << ","), it++) {
      if (!((*it).isNull())) {
	  (*it)->print(stream);
      } else {
	  stream << "NULL";
      }
  }
  stream << "\n";

  stream << tabs_next << "Left:\n";
  if (!left) {
      stream << tabs_next << "NULL\n";
  } else {
      left->print(stream, tab_num + 1);
      stream << "\n";
  }
  stream << tabs_next << "Right:\n";
  if (!right) {
      stream << tabs_next << "NULL\n";
  } else {
      right->print(stream, tab_num + 1);
      stream << "\n";
  }

  stream << tabs_next << "addedPathCond =";
  for (std::vector<UpdateRelation>::iterator it = appendedUpdateRelationsList.begin(); it != appendedUpdateRelationsList.end(); (stream << ","), it++) {
      it->print(stream);
  }
  stream << "\n";

  stream << tabs_next << "pathCond =";
  for (std::vector<UpdateRelation>::iterator it = updateRelationsList.begin(); it != updateRelationsList.end(); (stream << ","), it++) {
      it->print(stream);
  }
  stream << "\n";

  stream << tabs_next << "dependenciesLoc =";
  for (std::vector< ref<Expr> >::iterator it = dependenciesLoc.begin(); it != dependenciesLoc.end(); (stream << ","), it++) {
      if (!((*it).isNull())) {
	  (*it)->print(stream);
      } else {
	  stream << "NULL";
      }
  }
  stream << "\n";

  stream << tabs_next << "interpolant =";
  if (interpolant.isNull()) {
      stream << "NULL\n";
  } else {
      interpolant->print(stream);
      stream << "\n";
  }

  stream << tabs_next << "InterpolantStatus =" << InterpolantStatus;

}



