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

void PathConditionMarker::includeInInterpolant() {
  if (mayBeInInterpolant) {
      pathCondition->includeInInterpolant();
  }
}

/**/

PathCondition::PathCondition(ref<Expr> &constraint, Dependency *dependency,
                             llvm::Value *condition, PathCondition *prev)
    : constraint(constraint), shadowConstraint(constraint), shadowed(false),
      dependency(dependency),
      condition(dependency ? dependency->getLatestValue(condition) : 0),
      inInterpolant(false), tail(prev) {}

PathCondition::~PathCondition() {}

ref<Expr> PathCondition::car() const {
  return constraint;
}

PathCondition *PathCondition::cdr() const {
  return tail;
}

void PathCondition::includeInInterpolant() {
  // We mark all values to which this constraint depends
  dependency->markAllValues(condition);

  // We mark this constraint itself as in the interpolant
  inInterpolant = true;
}

bool PathCondition::carInInterpolant() const {
  return inInterpolant;
}

std::vector< ref<Expr> > PathCondition::packInterpolant() {
  std::vector< ref<Expr> > res;
  for (PathCondition *it = this; it != 0; it = it->tail) {
      if (it->inInterpolant) {
	  if (!it->shadowed) {
	      it->constraint->dump();
	      it->shadowConstraint = ShadowArray::getShadowExpression(it->constraint);
	      it->shadowed = true;
	  }
	  res.push_back(it->shadowConstraint);
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
    : nodeId(node->getNodeId()), interpolant(node->getInterpolant()),
      singletonStore(node->getLatestCoreExpressions(true)),
      compositeStore(node->getCompositeCoreExpressions(true)) {
  for (std::map<llvm::Value *, ref<Expr> >::iterator
           it = singletonStore.begin(),
           itEnd = singletonStore.end();
       it != itEnd; ++it) {
    singletonStoreKeys.push_back(it->first);
  }

  for (std::map<llvm::Value *, std::vector<ref<Expr> > >::iterator
           it = compositeStore.begin(),
           itEnd = compositeStore.end();
       it != itEnd; ++it) {
    compositeStoreKeys.push_back(it->first);
  }
}

SubsumptionTableEntry::~SubsumptionTableEntry() {}

bool SubsumptionTableEntry::subsumed(TimingSolver *solver,
                                     ExecutionState& state,
                                     double timeout) {
  if (state.itreeNode == 0)
    return false;

  if (state.itreeNode->getNodeId() == nodeId) {

    // TODO: Existential variables not taken into account!
    std::map<llvm::Value *, ref<Expr> > stateSingletonStore =
        state.itreeNode->getLatestCoreExpressions();
    std::map<llvm::Value *, std::vector< ref<Expr> > > stateCompositeStore =
        state.itreeNode->getCompositeCoreExpressions();

    ref<Expr> stateEqualityConstraints = ConstantExpr::alloc(1, Expr::Bool);
    for (std::vector<llvm::Value *>::iterator it = singletonStoreKeys.begin(),
                                              itEnd = singletonStoreKeys.end();
         it != itEnd; ++it) {
      const ref<Expr> lhs = singletonStore[*it];
      const ref<Expr> rhs = stateSingletonStore[*it];
      stateEqualityConstraints =
          AndExpr::create(EqExpr::create(lhs, rhs), stateEqualityConstraints);
    }

    for (std::vector<llvm::Value *>::iterator it = compositeStoreKeys.begin(),
                                              itEnd = compositeStoreKeys.end();
         it != itEnd; ++it) {
      std::vector<ref<Expr> > lhsList = compositeStore[*it];
      std::vector<ref<Expr> > rhsList = stateCompositeStore[*it];

      ref<Expr> auxDisjuncts = ConstantExpr::alloc(0, Expr::Bool);
      bool auxDisjunctsEmpty = true;

      for (std::vector<ref<Expr> >::iterator lhsIter = lhsList.begin(),
                                             lhsIterEnd = lhsList.end();
           lhsIter != lhsIterEnd; ++lhsIter) {
        for (std::vector<ref<Expr> >::iterator rhsIter = rhsList.begin(),
                                               rhsIterEnd = rhsList.end();
             rhsIter != rhsIterEnd; ++rhsIter) {
          const ref<Expr> lhs = *lhsIter;
          const ref<Expr> rhs = *rhsIter;
          auxDisjuncts = OrExpr::create(EqExpr::create(lhs, rhs), auxDisjuncts);
          auxDisjunctsEmpty = false;
        }
      }

      if (!auxDisjunctsEmpty)
        stateEqualityConstraints =
            AndExpr::create(auxDisjuncts, stateEqualityConstraints);
    }

    // FIXME: To be removed later.
    return false;

    // We create path condition needed constraints marking structure
      std::map< ref<Expr>, PathConditionMarker *> markerMap =
	  state.itreeNode->makeMarkerMap();

      for (std::vector< ref<Expr> >::iterator it0 = interpolant.begin();
	  it0 != interpolant.end(); it0++) {
	  ref<Expr> query = *it0;
	  Solver::Validity result;

          // llvm::errs() << "Querying for subsumption check:\n";
          // ExprPPrinter::printQuery(llvm::errs(), state.constraints, query);

          solver->setTimeout(timeout);
          bool success = solver->evaluate(state, query, result);
          solver->setTimeout(0);
          if (success && result == Solver::True) {
            std::vector<ref<Expr> > unsatCore = solver->getUnsatCore();

            for (std::vector<ref<Expr> >::iterator it1 = unsatCore.begin();
                 it1 != unsatCore.end(); it1++) {
              markerMap[*it1]->mayIncludeInInterpolant();
            }

          } else {
            return false;
          }
      }

      // State subsumed, we mark needed constraints on the
      // path condition.
      for (std::map< ref<Expr>, PathConditionMarker *>::iterator it = markerMap.begin();
	  it != markerMap.end(); it++) {
	  it->second->includeInInterpolant();
      }
      markerMap.clear();
      return true;
  }
  return false;
}

void SubsumptionTableEntry::dump() const {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void SubsumptionTableEntry::print(llvm::raw_ostream &stream) const {
  stream << "------------ Subsumption Table Entry ------------\n";
  stream << "Program point = " << nodeId << "\n";
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
    if (!node->isSubsumed && node->introducesMarkedConstraint()) {
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
  if (unsatCore.size() == 0)
      return;

  llvm::BranchInst *binst =
      llvm::dyn_cast<llvm::BranchInst>(state.prevPC->inst);
  if (binst) {
    currentINode->dependency->markAllValues(
        currentINode->dependency->getLatestValue(binst->getCondition()));
  }

  // Process the unsat core in case it was computed (non-empty)
  PathCondition *pc = currentINode->pathCondition;

  if (pc != 0) {
    for (std::vector<ref<Expr> >::reverse_iterator it = unsatCore.rbegin();
         it != unsatCore.rend(); it++) {
      while (pc != 0) {
        if (pc->car().compare(it->get()) == 0) {
          pc->includeInInterpolant();
          pc = pc->cdr();
          break;
        }
        pc = pc->cdr();
      }
      if (pc == 0)
        break;
      }
  }
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
  llvm::errs() << "------------------------- ITree Structure ---------------------------\n";
  stream << this->root->nodeId;
  if (this->root == this->currentINode) {
      stream << " (active)";
  }
  this->printNode(stream, this->root, "");
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

unsigned int ITreeNode::getNodeId() {
  return nodeId;
}

std::vector< ref<Expr> > ITreeNode::getInterpolant() const {
  return this->pathCondition->packInterpolant();
}

void ITreeNode::setNodeLocation(unsigned int programPoint) {
  if (this->nodeId == 0)  {
    this->nodeId = programPoint;
  }
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

std::map< ref<Expr>, PathConditionMarker *> ITreeNode::makeMarkerMap() {
  std::map< ref<Expr>, PathConditionMarker *> result;
  for (PathCondition *it = pathCondition; it != 0; it = it->cdr()) {
      result.insert( std::pair< ref<Expr>, PathConditionMarker *>
	(it->car(), new PathConditionMarker(it)) );
  }
  return result;
}

bool ITreeNode::introducesMarkedConstraint() {
  if (parent != 0 &&
      pathCondition != parent->pathCondition &&
      pathCondition->carInInterpolant()) {
      return true;
  }
  return false;
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
ITreeNode::getLatestCoreExpressions(bool interpolantValueOnly) const {
  return dependency->getLatestCoreExpressions(interpolantValueOnly);
}

std::map<llvm::Value *, std::vector<ref<Expr> > >
ITreeNode::getCompositeCoreExpressions(bool interpolantValueOnly) const {
  return dependency->getCompositeCoreExpressions(interpolantValueOnly);
}

void ITreeNode::dump() const {
  llvm::errs() << "\n------------------------- ITree Node --------------------------------\n";
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void ITreeNode::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void ITreeNode::print(llvm::raw_ostream &stream, const unsigned int tab_num) const {
  std::string tabs = makeTabs(tab_num);
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
  if (dependency) {
    stream << tabs_next << "------- Abstract Dependencies ----------\n";
    dependency->print(stream, tab_num + 1);
  }
}
