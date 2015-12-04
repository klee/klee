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


PathCondition::PathCondition(ref<Expr>& constraint) :
    constraint(constraint), inInterpolant(false), tail(0) {}

PathCondition::PathCondition(ref<Expr>& constraint, PathCondition *prev) :
    constraint(constraint), inInterpolant(false), tail(prev) {}

PathCondition::~PathCondition() {
  delete tail;
}

ref<Expr> PathCondition::car() const {
  return constraint;
}

PathCondition *PathCondition::cdr() const {
  return tail;
}

void PathCondition::includeInInterpolant() {
  inInterpolant = true;
}

std::vector< ref<Expr> > PathCondition::pack() const {
  std::vector< ref<Expr> > res;
  for (const PathCondition *it = this; it != 0; it = it->tail) {
      res.push_back(it->constraint);
  }
  return res;
}

void PathCondition::dump() {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void PathCondition::print(llvm::raw_ostream& stream) {
  stream << "[";
  for (PathCondition *it = this; it != 0; it = it->tail) {
      it->constraint->print(stream);
      stream << ": " << (it->inInterpolant ? "interpolant constraint" : "non-interpolant constraint");
      if (it->tail != 0) stream << ",";
  }
  stream << "]";
}

SubsumptionTableEntry::SubsumptionTableEntry(ITreeNode *node) :
  programPoint(node->programPoint),
  interpolant(node->getInterpolant()) {}

SubsumptionTableEntry::~SubsumptionTableEntry() {}

bool SubsumptionTableEntry::subsumed(ITreeNode *state) {
  /// We simply return false for now
  return false;
}

void SubsumptionTableEntry::dump() const {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void SubsumptionTableEntry::print(llvm::raw_ostream &stream) const {
  stream << "------------ Subsumption Table Entry ------------\n";
  stream << "Program point = " << programPoint << "\n";
  stream << "interpolant = [";
  for (std::vector< ref<Expr> >::const_iterator it = interpolant.begin();
      it != interpolant.end(); it++) {
      it->get()->print(stream);
      if (it + 1 != interpolant.end()) {
	  stream << ",";
      }
  }
  stream << "]\n";
}

ITree::ITree(ExecutionState* _root) :
    currentINode(0),
    root(new ITreeNode(0, _root)) {}

ITree::~ITree() {}

void ITree::checkCurrentNodeSubsumption() {
  assert(currentINode != 0);

  for (std::vector<SubsumptionTableEntry>::iterator it = subsumptionTable.begin();
      it != subsumptionTable.end(); it++) {
      if (it->subsumed(currentINode)) {
	  currentINode->isSubsumed = true;
	  return;
      }
  }
}

std::vector<SubsumptionTableEntry> ITree::getStore() {
  return subsumptionTable;
}

void ITree::store(SubsumptionTableEntry subItem) {
  llvm::errs() << "TABLING: ";
  subItem.dump();
  llvm::errs() << "SIZE BEFORE: " << subsumptionTable.size() << "\n";
  subsumptionTable.push_back(subItem);
  llvm::errs() << "SIZE AFTER: " << subsumptionTable.size() << "\n";
}

bool ITree::isCurrentNodeSubsumed() {
  return currentINode? currentINode->isSubsumed : false;
}

void ITree::setCurrentINode(ITreeNode *node) {
  currentINode = node;
}

void ITree::markPathCondition(std::vector< std::pair< size_t, ref<Expr> > > unsat_core) {
  /// Simply return in case the unsatisfiability core is empty
  if (unsat_core.size() == 0)
	return;

  /// Process the unsat core in case it was computed (non-empty)
  PathCondition *pc = currentINode->pathCondition;

  if (pc != 0) {
      for (std::vector< std::pair<size_t, ref<Expr> > >::reverse_iterator it = unsat_core.rbegin();
	  it != unsat_core.rend(); it++) {
	  while (pc != 0) {
	      if (pc->car().compare(it->second.get()) == 0) {
		  pc->includeInInterpolant();
		  pc = pc->cdr();
		  break;
	      }
	      pc = pc->cdr();
	  }
	  if (pc == 0) break;
      }
  }
}

ITreeNode::ITreeNode(ITreeNode *_parent,
                     ExecutionState *_data)
: parent(_parent),
  left(0),
  right(0),
  programPoint(0),
  data(_data),
  isSubsumed(false) {

  pathCondition = (_parent != 0) ? _parent->pathCondition : 0;

  if (!(_data->constraints.empty())) {
      ref<Expr> lastConstraint = _data->constraints.back();
      if (pathCondition == 0) {
	  pathCondition = new PathCondition(lastConstraint);
      } else if (pathCondition->car().compare(lastConstraint) != 0) {
	  pathCondition = new PathCondition(lastConstraint, pathCondition);
      }
  }
}

ITreeNode::~ITreeNode() {
  delete pathCondition;
}

std::vector< ref<Expr> > ITreeNode::getInterpolant() const {
  return this->pathCondition->pack();
}

void ITreeNode::correctNodeLocation(unsigned int programPoint) {
  this->programPoint = programPoint;
}

void ITreeNode::split(ExecutionState *leftData, ExecutionState *rightData) {
  assert (left == 0 && right == 0);
  leftData->itreeNode = left = new ITreeNode(this, leftData);
  rightData->itreeNode = right = new ITreeNode(this, rightData);
}

ITreeNode *ITreeNode::getParent() {
  return parent;
}

ITreeNode *ITreeNode::getLeft() {
  return left;
}

ITreeNode *ITreeNode::getRight() {
  return right;
}

void ITreeNode::dump() const {
  llvm::errs() << "\n------------------------- Root ITree --------------------------------\n";
  this->print(llvm::errs());
}

void ITreeNode::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void ITreeNode::print(llvm::raw_ostream &stream, const unsigned int tab_num) const {
  std::string tabs = make_tabs(tab_num);
  std::string tabs_next = tabs + "\t";

  stream << tabs << "ITreeNode\n";
  stream << tabs_next << "programPoint = " << programPoint << "\n";
  stream << tabs_next << "pathCondition = ";
  if (pathCondition == 0) {
      stream << "NULL";
  } else {
      pathCondition->print(stream);
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
}



