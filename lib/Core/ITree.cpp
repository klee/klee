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


ConstraintList::ConstraintList(ref<Expr>& constraint) :
    constraint(constraint), tail(0) {}

ConstraintList::ConstraintList(ref<Expr>& constraint, ConstraintList *prev) :
    constraint(constraint), tail(prev) {}

ConstraintList::~ConstraintList() {
  delete tail;
}

ref<Expr> ConstraintList::car() const {
  return constraint;
}

ConstraintList *ConstraintList::cdr() const {
  return tail;
}

std::vector< ref<Expr> > ConstraintList::pack() const {
  std::vector< ref<Expr> > res;
  for (const ConstraintList *it = this; it != 0; it = it->tail) {
      res.push_back(it->constraint);
  }
  return res;
}

void ConstraintList::dump() {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void ConstraintList::print(llvm::raw_ostream& stream) {
  stream << "[";
  for (ConstraintList *it = this; it != 0; it = it->tail) {
      it->constraint->print(stream);
      if (it->tail != 0) stream << ",";
  }
  stream << "]";
}


UpdateRelation::UpdateRelation(const ref<Expr>& baseLoc, const ref<Expr>& value, const Operation& operationName) :
    base(0), valueLoc(0) {
  this->baseLoc = baseLoc;
  this->value = value;
  this->operationName = operationName;
}

UpdateRelation::~UpdateRelation() {}

ref<Expr> UpdateRelation::makeExpr(ref<Expr> locToCompare, ref<Expr>& lhs) const {
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

bool UpdateRelation::isBase(ref<Expr> expr) const {
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

ITreeNode::ITreeNode(ITreeNode *_parent,
                     ExecutionState *_data)
: interpolant(NULL),
  interpolantStatus(NoInterpolant),
  parent(_parent),
  left(0),
  right(0),
  programPoint(0),
  data(_data),
  isSubsumed(false) {

  constraintList = (_parent != 0) ? _parent->constraintList : 0;

  if (!(_data->constraints.empty())) {
      ref<Expr> lastConstraint = _data->constraints.back();
      if (constraintList == 0) {
	  constraintList = new ConstraintList(lastConstraint);
      } else if (constraintList->car().compare(lastConstraint) != 0) {
	  constraintList = new ConstraintList(lastConstraint, constraintList);
      }
  }
}

ITreeNode::~ITreeNode() {
  delete constraintList;
}

void ITreeNode::addUpdateRelations(std::vector<UpdateRelation> addedUpdateRelations) {
  std::vector<UpdateRelation>::iterator it;
  updateRelationsList.insert(it, addedUpdateRelations.begin(), addedUpdateRelations.end());
}

void ITreeNode::addUpdateRelations(ITreeNode *other) {
  std::vector<UpdateRelation>::iterator it;
  updateRelationsList.insert(it, other->updateRelationsList.begin(),
                             other->updateRelationsList.end());
}

void ITreeNode::addNewUpdateRelation(UpdateRelation& updateRelation) {
  updateRelationsList.push_back(updateRelation);
  newUpdateRelationsList.push_back(updateRelation);
}

void ITreeNode::addStoredNewUpdateRelationsTo(std::vector<UpdateRelation>& relationsList) {
  std::vector<UpdateRelation>::iterator it = relationsList.begin();
  relationsList.insert(it, newUpdateRelationsList.begin(), newUpdateRelationsList.end());
}

ref<Expr> ITreeNode::buildUpdateExpression(ref<Expr>& lhs, ref<Expr> rhs) {
  return klee::buildUpdateExpression(updateRelationsList, lhs, rhs);
}

ref<Expr> ITreeNode::buildNewUpdateExpression(ref<Expr>& lhs, ref<Expr> rhs) {
  return klee::buildUpdateExpression(newUpdateRelationsList, lhs, rhs);
}

ref<Expr> ITreeNode::getInterpolantBaseLocation(ref<Expr>& interpolant) {
  /// Get the base location from base
  for (std::vector<UpdateRelation>::const_iterator it = updateRelationsList.begin();
      it != updateRelationsList.end(); ++it) {
      /// To search the variable from the TransferRelation: For example, we
      /// have variable x within the constraint x <= 0 as a variable that we
      /// are looking for. In this case, x is in the lhs of the expression,
      /// so its index is 0, and the variable is the expression
      /// tmpInterpolant->getKid(0), If we have found x then we save its
      /// reference (baseLocation = it->baseLoc).
      if (it->isBase(interpolant->getKid(0))) {
	  return it->getBaseLoc();
      }
  }
  return 0;
}

void ITreeNode::setInterpolantStatus(Status interpolantStatus) {
  this->interpolantStatus = interpolantStatus;
}

void ITreeNode::setInterpolant(ref<Expr> interpolant) {
  this->interpolant = interpolant;
}

void ITreeNode::setInterpolant(ref<Expr> interpolant,
                                   Status interpolantStatus) {
  this->interpolant = interpolant;
  this->interpolantStatus = interpolantStatus;
}

void ITreeNode::setInterpolant(ref<Expr> interpolant,
                                   std::pair< ref<Expr>, ref<Expr> > interpolantLoc,
                                   Status interpolantStatus) {
  this->interpolant = interpolant;
  this->interpolantLoc = interpolantLoc;
  this->interpolantStatus = interpolantStatus;
}

std::vector< ref<Expr> > ITreeNode::getInterpolant() const {
  return this->constraintList->pack();
}

std::pair< ref<Expr>, ref<Expr> > ITreeNode::getInterpolantLoc() {
  return std::make_pair(this->interpolantLoc.first, this->interpolantLoc.second);
}

Status ITreeNode::getInterpolantStatus() {
  return this->interpolantStatus;
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
  stream << tabs_next << "constraintList = ";
  if (constraintList == 0) {
      stream << "NULL";
  } else {
      constraintList->print(stream);
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

  stream << tabs_next << "newUpdateRelationsList =";
  for (std::vector<UpdateRelation>::const_iterator it = newUpdateRelationsList.begin();
      it != newUpdateRelationsList.end(); (stream << ","), it++) {
      it->print(stream);
  }
  stream << "\n";

  stream << tabs_next << "updateRelationsList =";
  for (std::vector<UpdateRelation>::const_iterator it = updateRelationsList.begin();
      it != updateRelationsList.end(); (stream << ","), it++) {
      it->print(stream);
  }
  stream << "\n";

  stream << tabs_next << "dependenciesLoc =";
  for (std::vector< ref<Expr> >::const_iterator it = dependenciesLoc.begin();
      it != dependenciesLoc.end(); (stream << ","), it++) {
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

  stream << tabs_next << "interpolantLoc = pair(";
  if (!interpolantLoc.first.isNull()) {
      interpolantLoc.first->print(stream);
  } else {
      stream << "NULL";
  }
  stream << ",";
  if (!interpolantLoc.second.isNull()) {
      interpolantLoc.second->print(stream);
  } else {
      stream << "NULL";
  }
  stream << ")\n";
  stream << tabs_next << "InterpolantStatus =" << interpolantStatus << "\n";

}

ref<Expr> klee::buildUpdateExpression(std::vector<UpdateRelation> updateRelationsList, ref<Expr>& lhs, ref<Expr> rhs) {
  for (std::vector<UpdateRelation>::const_iterator pc = updateRelationsList.begin();
      pc != updateRelationsList.end(); ++pc) {
      rhs = pc->makeExpr(lhs, rhs);
  }
  return rhs;
}



