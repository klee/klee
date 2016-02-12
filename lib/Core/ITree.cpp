/*
 * ITree.cpp
 *
 *  Created on: Oct 15, 2015
 *      Author: felicia
 */

#include "ITree.h"
#include "TimingSolver.h"
#include "Dependency.h"

#include <klee/Expr.h>
#include <klee/Solver.h>
#include <klee/util/ExprPPrinter.h>
#include <vector>

using namespace klee;

// Interpolation is enabled by default
bool InterpolationOption::interpolation = true;

/**/

PathConditionMarker::PathConditionMarker(PathCondition *pathCondition) :
  mayBeInInterpolant(false), pathCondition(pathCondition) {}

PathConditionMarker::~PathConditionMarker() {}

void PathConditionMarker::mayIncludeInInterpolant() {
  mayBeInInterpolant = true;
}

void PathConditionMarker::includeInInterpolant(AllocationGraph *g) {
  if (mayBeInInterpolant) {
    pathCondition->includeInInterpolant(g);
  }
}

/**/

PathCondition::PathCondition(ref<Expr> &constraint, Dependency *dependency,
                             llvm::Value *condition, PathCondition *prev)
    : constraint(constraint), shadowConstraint(constraint), shadowed(false),
      dependency(dependency),
      condition(dependency ? dependency->getLatestValue(condition, constraint)
                           : 0),
      inInterpolant(false), tail(prev) {}

PathCondition::~PathCondition() {}

ref<Expr> PathCondition::car() const {
  return constraint;
}

PathCondition *PathCondition::cdr() const {
  return tail;
}

void PathCondition::includeInInterpolant(AllocationGraph *g) {
  // We mark all values to which this constraint depends
  dependency->markAllValues(g, condition);

  // We mark this constraint itself as in the interpolant
  inInterpolant = true;
}

bool PathCondition::carInInterpolant() const {
  return inInterpolant;
}

ref<Expr>
PathCondition::packInterpolant(std::vector<const Array *> &replacements) {
  ref<Expr> res;
  for (PathCondition *it = this; it != 0; it = it->tail) {
      if (it->inInterpolant) {
	  if (!it->shadowed) {
            it->shadowConstraint =
                ShadowArray::getShadowExpression(it->constraint, replacements);
            it->shadowed = true;
          }
          if (res.get()) {
            res = AndExpr::alloc(res, it->shadowConstraint);
          } else {
            res = it->shadowConstraint;
          }
      }
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

/**/

SubsumptionTableEntry::SubsumptionTableEntry(ITreeNode *node)
    : nodeId(node->getNodeId()) {
  std::vector<const Array *> replacements;

  interpolant = node->getInterpolant(replacements);

  singletonStore = node->getLatestInterpolantCoreExpressions(replacements);
  for (std::map<llvm::Value *, ref<Expr> >::iterator
           it = singletonStore.begin(),
           itEnd = singletonStore.end();
       it != itEnd; ++it) {
    singletonStoreKeys.push_back(it->first);
  }

  compositeStore = node->getCompositeInterpolantCoreExpressions(replacements);
  for (std::map<llvm::Value *, std::vector<ref<Expr> > >::iterator
           it = compositeStore.begin(),
           itEnd = compositeStore.end();
       it != itEnd; ++it) {
    compositeStoreKeys.push_back(it->first);
  }

  existentials = replacements;
}

SubsumptionTableEntry::~SubsumptionTableEntry() {}

ref<Expr> SubsumptionTableEntry::simplifyArithmeticBody(ref<Expr> existsExpr) {
  assert(ExistsExpr::classof(existsExpr.get()));
  ExistsExpr *expr = static_cast<ExistsExpr *>(existsExpr.get());
  ref<Expr> ret;

  std::vector<const Array *> boundVariables = expr->variables;
  ref<Expr> body = expr->body;

  // Do something with body and boundVariables
  // Currently the return values is just the argument itself.
  ret = existsExpr;

  return ret;
}

ref<Expr> SubsumptionTableEntry::simplifyExistsExpr(ref<Expr> existsExpr) {
  assert(ExistsExpr::classof(existsExpr.get()));

  ref<Expr> ret = simplifyArithmeticBody(existsExpr);
  if (!ExistsExpr::classof(ret.get()))
    return ret;

  return ret;
}

bool SubsumptionTableEntry::subsumed(TimingSolver *solver,
                                     ExecutionState& state,
                                     double timeout) {
  // Check if we are at the right program point
  if (state.itreeNode == 0 || reinterpret_cast<uintptr_t>(state.pc->inst) !=
                                  state.itreeNode->getNodeId() ||
      state.itreeNode->getNodeId() != nodeId)
    return false;

  // Quick check for subsumption in case the interpolant is empty
  if (empty()) return true;

  std::map<llvm::Value *, ref<Expr> > stateSingletonStore =
      state.itreeNode->getLatestCoreExpressions();
  std::map<llvm::Value *, std::vector<ref<Expr> > > stateCompositeStore =
      state.itreeNode->getCompositeCoreExpressions();

  ref<Expr> stateEqualityConstraints;
  for (std::vector<llvm::Value *>::iterator
           itBegin = singletonStoreKeys.begin(),
           itEnd = singletonStoreKeys.end(), it = itBegin;
       it != itEnd; ++it) {
      const ref<Expr> lhs = singletonStore[*it];
      const ref<Expr> rhs = stateSingletonStore[*it];
      stateEqualityConstraints =
          (it == itBegin ? EqExpr::alloc(lhs, rhs)
                         : AndExpr::alloc(EqExpr::alloc(lhs, rhs),
                                          stateEqualityConstraints));
  }

  for (std::vector<llvm::Value *>::iterator it = compositeStoreKeys.begin(),
                                            itEnd = compositeStoreKeys.end();
       it != itEnd; ++it) {
      std::vector<ref<Expr> > lhsList = compositeStore[*it];
      std::vector<ref<Expr> > rhsList = stateCompositeStore[*it];

      ref<Expr> auxDisjuncts;
      bool auxDisjunctsEmpty = true;

      for (std::vector<ref<Expr> >::iterator lhsIter = lhsList.begin(),
                                             lhsIterEnd = lhsList.end();
           lhsIter != lhsIterEnd; ++lhsIter) {
        for (std::vector<ref<Expr> >::iterator rhsIter = rhsList.begin(),
                                               rhsIterEnd = rhsList.end();
             rhsIter != rhsIterEnd; ++rhsIter) {
          const ref<Expr> lhs = *lhsIter;
          const ref<Expr> rhs = *rhsIter;

          if (auxDisjunctsEmpty) {
            auxDisjuncts = EqExpr::alloc(lhs, rhs);
            auxDisjunctsEmpty = false;
          } else {
            auxDisjuncts = OrExpr::alloc(EqExpr::alloc(lhs, rhs), auxDisjuncts);
          }
        }
      }

      if (!auxDisjunctsEmpty)
        stateEqualityConstraints =
            AndExpr::alloc(auxDisjuncts, stateEqualityConstraints);
  }

  // We create path condition needed constraints marking structure
  std::map<ref<Expr>, PathConditionMarker *> markerMap =
      state.itreeNode->makeMarkerMap();

  Solver::Validity result;
  ref<Expr> query = stateEqualityConstraints.get()
                        ? AndExpr::alloc(interpolant, stateEqualityConstraints)
                        : interpolant;

  if (!existentials.empty()) {
    query = simplifyExistsExpr(ExistsExpr::create(existentials, query));
  }

  // llvm::errs() << "Querying for subsumption check:\n";
  // ExprPPrinter::printQuery(llvm::errs(), state.constraints, query);

  bool success = false;

  Z3Solver *z3solver = 0;

  if (!existentials.empty()) {
      // Instantiate a new Z3 solver to make sure we use Z3
      // without pre-solving optimizations. It would be nice
      // in the future to just run solver->evaluate so that
      // the optimizations can be used, but this requires
      // handling of quantified expressions by KLEE's pre-solving
      // procedure, which does not exist currently.
      z3solver = new Z3Solver();

      z3solver->setCoreSolverTimeout(timeout);
      success = z3solver->directComputeValidity(Query(state.constraints, query),
                                                result);
      z3solver->setCoreSolverTimeout(0);
  } else {
      // We call the solver in the standard way if the
      // formula is unquantified.
      solver->setTimeout(timeout);
      success = solver->evaluate(state, query, result);
      solver->setTimeout(0);
  }

  if (success && result == Solver::True) {
      // llvm::errs() << "Solver decided validity\n";
      std::vector<ref<Expr> > unsatCore;
      if (z3solver) {
        unsatCore = z3solver->getUnsatCore();
        delete z3solver;
      } else {
        unsatCore = solver->getUnsatCore();
      }

      for (std::vector<ref<Expr> >::iterator it1 = unsatCore.begin();
           it1 != unsatCore.end(); it1++) {
        markerMap[*it1]->mayIncludeInInterpolant();
      }

  } else {
      if (z3solver)
        delete z3solver;
      return false;
  }

  // State subsumed, we mark needed constraints on the
  // path condition.
  AllocationGraph *g = new AllocationGraph();
  for (std::map<ref<Expr>, PathConditionMarker *>::iterator
           it = markerMap.begin(),
           itEnd = markerMap.end();
       it != itEnd; it++) {
    it->second->includeInInterpolant(g);
  }
  ITreeNode::deleteMarkerMap(markerMap);

  // llvm::errs() << "AllocationGraph\n";
  // g->dump();

  // We mark memory allocations needed for the unsatisfiabilty core
  state.itreeNode->computeInterpolantAllocations(g);

  delete g; // Delete the AllocationGraph object
  return true;
}

void SubsumptionTableEntry::dump() const {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void SubsumptionTableEntry::print(llvm::raw_ostream &stream) const {
  stream << "------------ Subsumption Table Entry ------------\n";
  stream << "Program point = " << nodeId << "\n";
  stream << "interpolant = ";
  if (interpolant.get())
    interpolant->print(stream);
  else
    stream << "(empty)";
  stream << "\n";

  if (!singletonStore.empty()) {
    stream << "singleton allocations = [";
    for (std::map<llvm::Value *, ref<Expr> >::const_iterator
             itBegin = singletonStore.begin(),
             itEnd = singletonStore.end(), it = itBegin;
         it != itEnd; ++it) {
      if (it != itBegin)
        stream << ",";
      stream << "(";
      it->first->print(stream);
      stream << ",";
      it->second->print(stream);
      stream << ")";
    }
    stream << "]\n";
  }

  if (!compositeStore.empty()) {
    stream << "composite allocations = [";
    for (std::map<llvm::Value *, std::vector<ref<Expr> > >::const_iterator
             it0Begin = compositeStore.begin(),
             it0End = compositeStore.end(), it0 = it0Begin;
         it0 != it0End; ++it0) {
      if (it0 != it0Begin)
        stream << ",";
      stream << "(";
      it0->first->print(stream);
      stream << ",[";
      for (std::vector<ref<Expr> >::const_iterator
               it1Begin = it0->second.begin(),
               it1End = it0->second.end(), it1 = it1Begin;
           it1 != it1End; ++it1) {
        if (it1 != it1Begin)
          stream << ",";
        (*it1)->print(stream);
      }
      stream << "])";
    }
    stream << "]\n";
  }

  if (!existentials.empty()) {
    stream << "existentials = [";
    for (std::vector<const Array *>::const_iterator
             itBegin = existentials.begin(),
             itEnd = existentials.end(), it = itBegin;
         it != itEnd; ++it) {
      if (it != itBegin)
        stream << ", ";
      stream << (*it)->name;
    }
    stream << "]\n";
  }
}

/**/

ITree::ITree(ExecutionState *_root) {
  currentINode = 0;
  if (!_root->itreeNode) {
    currentINode = new ITreeNode(0);
  }
  root = currentINode;
}

ITree::~ITree() {}

bool ITree::checkCurrentStateSubsumption(TimingSolver *solver,
                                         ExecutionState& state,
                                         double timeout) {
  assert(state.itreeNode == currentINode);

  for (std::vector<SubsumptionTableEntry>::iterator it = subsumptionTable.begin();
      it != subsumptionTable.end(); it++) {

      if (it->subsumed(solver, state, timeout)) {

        // We mark as subsumed such that the node will not be
        // stored into table (the table already contains a more
        // general entry).
        currentINode->isSubsumed = true;

        return true;
      }
  }
  return false;
}

std::vector<SubsumptionTableEntry> ITree::getStore() {
  return subsumptionTable;
}

void ITree::store(SubsumptionTableEntry subItem) {
  subsumptionTable.push_back(subItem);
}

void ITree::setCurrentINode(ITreeNode *node) {
  currentINode = node;
}

void ITree::remove(ITreeNode *node) {
  assert(!node->left && !node->right);
  do {
    ITreeNode *p = node->parent;

    // As the node is about to be deleted, it must have been completely
    // traversed, hence the correct time to table the interpolant.
    if (!node->isSubsumed) {
      SubsumptionTableEntry entry(node);
      store(entry);
    }

    delete node;
    if (p) {
      if (node == p->left) {
        p->left = 0;
      } else {
        assert(node == p->right);
        p->right = 0;
      }
    }
    node = p;
  } while (node && !node->left && !node->right);
}

std::pair<ITreeNode *, ITreeNode *> ITree::split(ITreeNode *parent, ExecutionState *left, ExecutionState *right) {
  parent->split(left, right);
  return std::pair<ITreeNode *, ITreeNode *> (parent->left, parent->right);
}

void ITree::markPathCondition(ExecutionState &state, TimingSolver *solver) {
  std::vector<ref<Expr> > unsatCore = solver->getUnsatCore();

  // Simply return in case the unsatisfiability core is empty
  if (unsatCore.empty())
      return;

  AllocationGraph *g = new AllocationGraph();

  llvm::BranchInst *binst =
      llvm::dyn_cast<llvm::BranchInst>(state.prevPC->inst);
  if (binst) {
    currentINode->dependency->markAllValues(g, binst->getCondition());
  }

  PathCondition *pc = currentINode->pathCondition;

  if (pc != 0) {
    for (std::vector<ref<Expr> >::reverse_iterator it = unsatCore.rbegin();
         it != unsatCore.rend(); it++) {
      while (pc != 0) {
        if (pc->car().compare(it->get()) == 0) {
          pc->includeInInterpolant(g);
          pc = pc->cdr();
          break;
        }
        pc = pc->cdr();
      }
      if (pc == 0)
        break;
      }
  }

  // llvm::errs() << "AllocationGraph\n";
  // g->dump();

  // Compute memory allocations needed by the unsatisfiability core
  currentINode->computeInterpolantAllocations(g);

  delete g; // Delete the AllocationGraph object
}

void ITree::executeAbstractBinaryDependency(llvm::Instruction *instr,
                                            ref<Expr> valueExpr,
                                            ref<Expr> tExpr, ref<Expr> fExpr) {
  currentINode->executeBinaryDependency(instr, valueExpr, tExpr, fExpr);
}

void ITree::executeAbstractMemoryDependency(llvm::Instruction *instr,
                                            ref<Expr> value,
                                            ref<Expr> address) {
  currentINode->executeAbstractMemoryDependency(instr, value, address);
}

void ITree::executeAbstractDependency(llvm::Instruction *instr,
                                      ref<Expr> value) {
  currentINode->executeAbstractDependency(instr, value);
}

void ITree::printNode(llvm::raw_ostream& stream, ITreeNode *n, std::string edges) {
  if (n->left != 0) {
    stream << "\n";
      stream << edges << "+-- L:" << n->left->nodeId;
      if (this->currentINode == n->left) {
	  stream << " (active)";
      }
      if (n->right != 0) {
	  printNode(stream, n->left, edges + "|   ");
      } else {
	  printNode(stream, n->left, edges + "    ");
      }
  }
  if (n->right != 0) {
    stream << "\n";
      stream << edges << "+-- R:" << n->right->nodeId;
      if (this->currentINode == n->right) {
	  stream << " (active)";
      }
      printNode(stream, n->right, edges + "    ");
  }
}

void ITree::print(llvm::raw_ostream& stream) {
  stream << "------------------------- ITree Structure "
            "---------------------------\n";
  stream << this->root->nodeId;
  if (this->root == this->currentINode) {
      stream << " (active)";
  }
  this->printNode(stream, this->root, "");
  stream << "\n------------------------- Subsumption Table "
            "-------------------------\n";
  for (std::vector<SubsumptionTableEntry>::iterator
           it = subsumptionTable.begin(),
           itEnd = subsumptionTable.end();
       it != itEnd; ++it) {
    (*it).print(stream);
  }
}

void ITree::dump() {
  this->print(llvm::errs());
}

/**/

ITreeNode::ITreeNode(ITreeNode *_parent)
    : parent(_parent), left(0), right(0), nodeId(0), isSubsumed(false) {

  pathCondition = (_parent != 0) ? _parent->pathCondition : 0;

  // Inherit the abstract dependency or NULL
  dependency = new Dependency(_parent ? _parent->dependency : 0);
}

ITreeNode::~ITreeNode() {
  // Only delete the path condition if it's not
  // also the parent's path condition
  PathCondition *itEnd = parent ? parent->pathCondition : 0;

  PathCondition *it = pathCondition;
  while (it != itEnd) {
    PathCondition *tmp = it;
    it = it->cdr();
    delete tmp;
  }

  if (dependency)
    delete dependency;
}

uintptr_t ITreeNode::getNodeId() { return nodeId; }

ref<Expr>
ITreeNode::getInterpolant(std::vector<const Array *> &replacements) const {
  return this->pathCondition->packInterpolant(replacements);
}

void ITreeNode::setNodeLocation(uintptr_t programPoint) {
  if (!nodeId)
    nodeId = programPoint;
}

void ITreeNode::addConstraint(ref<Expr> &constraint, llvm::Value *condition) {
  pathCondition =
      new PathCondition(constraint, dependency, condition, pathCondition);
}

void ITreeNode::split(ExecutionState *leftData, ExecutionState *rightData) {
  assert (left == 0 && right == 0);
  leftData->itreeNode = left = new ITreeNode(this);
  rightData->itreeNode = right = new ITreeNode(this);
}

std::map< ref<Expr>, PathConditionMarker *> ITreeNode::makeMarkerMap() const {
  std::map< ref<Expr>, PathConditionMarker *> result;
  for (PathCondition *it = pathCondition; it != 0; it = it->cdr()) {
      result.insert( std::pair< ref<Expr>, PathConditionMarker *>
	(it->car(), new PathConditionMarker(it)) );
  }
  return result;
}

void ITreeNode::deleteMarkerMap(std::map<ref<Expr>, PathConditionMarker *>& markerMap) {
  for (std::map<ref<Expr>, PathConditionMarker *>::iterator it = markerMap.begin(),
      itEnd = markerMap.end(); it != itEnd; ++it) {
      delete it->second;
  }
  markerMap.clear();
}

void ITreeNode::executeBinaryDependency(llvm::Instruction *i,
                                        ref<Expr> valueExpr, ref<Expr> tExpr,
                                        ref<Expr> fExpr) {
  dependency->executeBinary(i, valueExpr, tExpr, fExpr);
}

void ITreeNode::executeAbstractMemoryDependency(llvm::Instruction *instr,
                                                ref<Expr> value,
                                                ref<Expr> address) {
  dependency->executeMemoryOperation(instr, value, address);
}

void ITreeNode::executeAbstractDependency(llvm::Instruction *instr,
                                          ref<Expr> value) {
  dependency->execute(instr, value);
}

void ITreeNode::bindCallArguments(llvm::Instruction *site,
                                  std::vector<ref<Expr> > &arguments) {
  dependency->bindCallArguments(site, arguments);
}

void ITreeNode::popAbstractDependencyFrame(llvm::CallInst *site,
                                           llvm::Instruction *inst,
                                           ref<Expr> returnValue) {
  // TODO: This is probably where we should simplify
  // the dependency graph by removing callee values.

  dependency->bindReturnValue(site, inst, returnValue);
}

std::map<llvm::Value *, ref<Expr> >
ITreeNode::getLatestCoreExpressions() const {
  std::map<llvm::Value *, ref<Expr> > ret;
  std::vector<const Array *> dummyReplacements;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret =
        parent->dependency->getLatestCoreExpressions(dummyReplacements, false);
  return ret;
}

std::map<llvm::Value *, std::vector<ref<Expr> > >
ITreeNode::getCompositeCoreExpressions() const {
  std::map<llvm::Value *, std::vector<ref<Expr> > > ret;
  std::vector<const Array *> dummyReplacements;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret = parent->dependency->getCompositeCoreExpressions(dummyReplacements,
                                                          false);
  return ret;
}

std::map<llvm::Value *, ref<Expr> >
ITreeNode::getLatestInterpolantCoreExpressions(
    std::vector<const Array *> &replacements) const {
  std::map<llvm::Value *, ref<Expr> > ret;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret = parent->dependency->getLatestCoreExpressions(replacements, true);
  return ret;
}

std::map<llvm::Value *, std::vector<ref<Expr> > >
ITreeNode::getCompositeInterpolantCoreExpressions(
    std::vector<const Array *> &replacements) const {
  std::map<llvm::Value *, std::vector<ref<Expr> > > ret;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret = parent->dependency->getCompositeCoreExpressions(replacements, true);
  return ret;
}

void ITreeNode::computeInterpolantAllocations(AllocationGraph *g) {
  dependency->computeInterpolantAllocations(g);
}

void ITreeNode::dump() const {
  llvm::errs() << "------------------------- ITree Node "
                  "--------------------------------\n";
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void ITreeNode::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void ITreeNode::print(llvm::raw_ostream &stream, const unsigned tabNum) const {
  std::string tabs = makeTabs(tabNum);
  std::string tabs_next = appendTab(tabs);

  stream << tabs << "ITreeNode\n";
  stream << tabs_next << "node Id = " << nodeId << "\n";
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
    left->print(stream, tabNum + 1);
      stream << "\n";
  }
  stream << tabs_next << "Right:\n";
  if (!right) {
      stream << tabs_next << "NULL\n";
  } else {
    right->print(stream, tabNum + 1);
      stream << "\n";
  }
  if (dependency) {
    stream << tabs_next << "------- Abstract Dependencies ----------\n";
    dependency->print(stream, tabNum + 1);
  }
}
