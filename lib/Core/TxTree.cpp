//===-- TxTree.cpp - Interpolation tree -------------------------*- C++ -*-===//
//
//               The Tracer-X KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the implementations of the classes for
/// interpolation and subsumption checks for search-space reduction.
///
//===----------------------------------------------------------------------===//

#include "TxTree.h"

#include "Dependency.h"
#include "ShadowArray.h"
#include "TimingSolver.h"
#include "TxPrintUtil.h"

#include <klee/CommandLine.h>
#include <klee/Expr.h>
#include <klee/Solver.h>
#include <klee/SolverStats.h>
#include <klee/Internal/Support/ErrorHandling.h>
#include <klee/util/ExprPPrinter.h>
#include <fstream>
#include <vector>

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 5)
#include <llvm/IR/DebugInfo.h>
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
#include <llvm/DebugInfo.h>
#else
#include <llvm/Analysis/DebugInfo.h>
#endif

using namespace klee;

/**/

std::string TxTreeGraph::NumberedEdge::render() const {
  std::ostringstream stream;
  stream << "Node" << source->nodeSequenceNumber << " -> Node"
         << destination->nodeSequenceNumber << " [style=dashed,label=\""
         << number << "\"];";
  return stream.str();
}

/**/

TxTreeGraph *TxTreeGraph::instance = 0;

std::string TxTreeGraph::recurseRender(TxTreeGraph::Node *node) {
  std::ostringstream stream;

  if (node->nodeSequenceNumber) {
    stream << "Node" << node->nodeSequenceNumber;
  } else {
    // Node sequence number is zero; this must be an internal node created due
    // to splitting in memory access
    if (!node->internalNodeId) {
      node->internalNodeId = (++internalNodeId);
    }
    stream << "InternalNode" << node->internalNodeId;
  }
  std::string sourceNodeName = stream.str();

  size_t pos = 0;
  std::string replacementName(node->name);
  std::stringstream repStream1;
  while ((pos = replacementName.find("{")) != std::string::npos) {
    if (pos != std::string::npos) {
      repStream1 << replacementName.substr(0, pos) << "\\{";
      replacementName = replacementName.substr(pos + 2);
    }
  }
  repStream1 << replacementName;
  replacementName = repStream1.str();
  std::stringstream repStream2;
  while ((pos = replacementName.find("}")) != std::string::npos) {
    if (pos != std::string::npos) {
      repStream2 << replacementName.substr(0, pos) << "\\}";
      replacementName = replacementName.substr(pos + 2);
    }
  }
  repStream2 << replacementName;
  replacementName = repStream2.str();

  stream << " [shape=record,label=\"{";
  if (node->nodeSequenceNumber) {
    stream << node->nodeSequenceNumber << ": " << replacementName;
  } else {
    // The internal node id must have been set earlier
    assert(node->internalNodeId && "id for internal node must have been set");
    stream << "Internal node " << node->internalNodeId << ": ";
  }
  stream << "\\l";
  for (std::map<PathCondition *, std::pair<std::string, bool> >::const_iterator
           it = node->pathConditionTable.begin(),
           ie = node->pathConditionTable.end();
       it != ie; ++it) {
    stream << it->second.first;
    if (it->second.second)
      stream << " ITP";
    stream << "\\l";
  }
  if (node->memoryError) {
    stream << "OUT-OF-BOUND: " << node->memoryErrorLocation << "\\l";
  }
  if (node->subsumed) {
    stream << "(subsumed)\\l";
  }
  if (node->falseTarget || node->trueTarget)
    stream << "|{<s0>F|<s1>T}";
  stream << "}\"];\n";

  if (node->falseTarget) {
    if (node->falseTarget->nodeSequenceNumber) {
      stream << sourceNodeName << ":s0 -> Node"
             << node->falseTarget->nodeSequenceNumber << ";\n";
    } else {
      if (!node->falseTarget->internalNodeId) {
        node->falseTarget->internalNodeId = (++internalNodeId);
      }
      stream << sourceNodeName << ":s0 -> InternalNode"
             << node->falseTarget->internalNodeId << ";\n";
    }
  }
  if (node->trueTarget) {
    if (node->trueTarget->nodeSequenceNumber) {
      stream << sourceNodeName + ":s1 -> Node"
             << node->trueTarget->nodeSequenceNumber << ";\n";
    } else {
      if (!node->trueTarget->internalNodeId) {
        node->trueTarget->internalNodeId = (++internalNodeId);
      }
      stream << sourceNodeName << ":s1 -> InternalNode"
             << node->trueTarget->internalNodeId << ";\n";
    }
  }
  if (node->falseTarget) {
    stream << recurseRender(node->falseTarget);
  }
  if (node->trueTarget) {
    stream << recurseRender(node->trueTarget);
  }
  return stream.str();
}

std::string TxTreeGraph::render() {
  std::string res("");

  // Simply return empty string when root is undefined
  if (!root)
    return res;

  std::ostringstream stream;
  for (std::vector<TxTreeGraph::NumberedEdge *>::iterator
           it = subsumptionEdges.begin(),
           ie = subsumptionEdges.end();
       it != ie; ++it) {
    stream << (*it)->render() << "\n";
  }

  res = "digraph search_tree {\n";
  res += recurseRender(root);
  res += stream.str();
  res += "}\n";
  return res;
}

TxTreeGraph::TxTreeGraph(TxTreeNode *_root)
    : subsumptionEdgeNumber(0), internalNodeId(0) {
  root = TxTreeGraph::Node::createNode();
  txTreeNodeMap[_root] = root;
}

TxTreeGraph::~TxTreeGraph() {
  if (root)
    delete root;

  txTreeNodeMap.clear();

  for (std::vector<TxTreeGraph::NumberedEdge *>::iterator
           it = subsumptionEdges.begin(),
           ie = subsumptionEdges.end();
       it != ie; ++it) {
    delete *it;
  }
  subsumptionEdges.clear();
}

void TxTreeGraph::addChildren(TxTreeNode *parent, TxTreeNode *falseChild,
                              TxTreeNode *trueChild) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(TxTreeGraph::instance && "Search tree graph not initialized");

  TxTreeGraph::Node *parentNode = instance->txTreeNodeMap[parent];

  parentNode->falseTarget = TxTreeGraph::Node::createNode();
  parentNode->trueTarget = TxTreeGraph::Node::createNode();
  instance->txTreeNodeMap[falseChild] = parentNode->falseTarget;
  instance->txTreeNodeMap[trueChild] = parentNode->trueTarget;
}

void TxTreeGraph::setCurrentNode(ExecutionState &state,
                                 const uint64_t _nodeSequenceNumber) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(TxTreeGraph::instance && "Search tree graph not initialized");

  TxTreeNode *txTreeNode = state.txTreeNode;
  TxTreeGraph::Node *node = instance->txTreeNodeMap[txTreeNode];
  if (!node->nodeSequenceNumber) {
    std::string functionName(
        state.pc->inst->getParent()->getParent()->getName().str());
    node->name = functionName + "\\l";
    llvm::raw_string_ostream out(node->name);
    if (llvm::MDNode *n = state.pc->inst->getMetadata("dbg")) {
      // Display the line, char position of this instruction
      llvm::DILocation loc(n);
      unsigned line = loc.getLineNumber();
      StringRef file = loc.getFilename();
      out << file << ":" << line << "\n";
    } else {
      state.pc->inst->print(out);
    }
    node->name = out.str();
    node->nodeSequenceNumber = _nodeSequenceNumber;
  }
}

void TxTreeGraph::markAsSubsumed(TxTreeNode *txTreeNode,
                                 SubsumptionTableEntry *entry) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(TxTreeGraph::instance && "Search tree graph not initialized");

  TxTreeGraph::Node *node = instance->txTreeNodeMap[txTreeNode];
  node->subsumed = true;
  TxTreeGraph::Node *subsuming = instance->tableEntryMap[entry];
  instance->subsumptionEdges.push_back(new TxTreeGraph::NumberedEdge(
      node, subsuming, ++(instance->subsumptionEdgeNumber)));
}

void TxTreeGraph::addPathCondition(TxTreeNode *txTreeNode,
                                   PathCondition *pathCondition,
                                   ref<Expr> condition) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(TxTreeGraph::instance && "Search tree graph not initialized");

  TxTreeGraph::Node *node = instance->txTreeNodeMap[txTreeNode];

  std::string s = PrettyExpressionBuilder::construct(condition);

  std::pair<std::string, bool> p(s, false);
  node->pathConditionTable[pathCondition] = p;
  instance->pathConditionMap[pathCondition] = node;
}

void TxTreeGraph::addTableEntryMapping(TxTreeNode *txTreeNode,
                                       SubsumptionTableEntry *entry) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(TxTreeGraph::instance && "Search tree graph not initialized");

  TxTreeGraph::Node *node = instance->txTreeNodeMap[txTreeNode];
  instance->tableEntryMap[entry] = node;
}

void TxTreeGraph::setAsCore(PathCondition *pathCondition) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(TxTreeGraph::instance && "Search tree graph not initialized");

  instance->pathConditionMap[pathCondition]
      ->pathConditionTable[pathCondition]
      .second = true;
}

void TxTreeGraph::setMemoryError(ExecutionState &state) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  TxTreeGraph::Node *node = instance->txTreeNodeMap[state.txTreeNode];
  node->memoryError = true;

  node->memoryErrorLocation = "";
  llvm::raw_string_ostream out(node->memoryErrorLocation);
  if (llvm::MDNode *n = state.pc->inst->getMetadata("dbg")) {
    // Display the line, char position of this instruction
    llvm::DILocation loc(n);
    unsigned line = loc.getLineNumber();
    StringRef file = loc.getFilename();
    out << file << ":" << line << "\n";
  } else {
    state.pc->inst->print(out);
  }
}

void TxTreeGraph::save(std::string dotFileName) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(TxTreeGraph::instance && "Search tree graph not initialized");

  std::string g(instance->render());
  std::ofstream out(dotFileName.c_str());
  if (!out.fail()) {
    out << g;
    out.close();
  }
}

/**/

PathCondition::PathCondition(ref<Expr> &constraint, Dependency *dependency,
                             llvm::Value *_condition,
                             const std::vector<llvm::Instruction *> &stack,
                             PathCondition *prev)
    : constraint(constraint), shadowConstraint(constraint), shadowed(false),
      dependency(dependency), core(false), tail(prev) {
  ref<VersionedValue> emptyCondition;
  if (dependency) {
    condition = dependency->getLatestValue(_condition, stack, constraint, true);
    assert(!condition.isNull() && "null constraint on path condition");
  } else {
    condition = emptyCondition;
  }
}

PathCondition::~PathCondition() {}

ref<Expr> PathCondition::car() const { return constraint; }

PathCondition *PathCondition::cdr() const { return tail; }

void PathCondition::setAsCore(int debugLevel) {
  // We mark all values to which this constraint depends
  std::string reason = "";
  if (debugLevel >= 1) {
    llvm::raw_string_ostream stream(reason);
    stream << "path condition [";
    constraint->print(stream);
    stream << "] of [";
    if (llvm::Instruction *inst =
            llvm::dyn_cast<llvm::Instruction>(condition->getValue())) {
      if (inst->getParent()->getParent()) {
        stream << inst->getParent()->getParent()->getName().str() << ": ";
      }
      if (llvm::MDNode *n = inst->getMetadata("dbg")) {
        llvm::DILocation loc(n);
        stream << loc.getLineNumber();
      } else {
        condition->getValue()->print(stream);
      }
    } else {
      condition->getValue()->print(stream);
    }
    stream << "]";
    stream.flush();
  }
  dependency->markAllValues(condition, reason);

  // We mark this constraint itself as core
  core = true;

  // We mark constraint as core in the search tree graph as well.
  TxTreeGraph::setAsCore(this);
}

bool PathCondition::isCore() const { return core; }

ref<Expr>
PathCondition::packInterpolant(std::set<const Array *> &replacements) {
  ref<Expr> res;
  for (PathCondition *it = this; it != 0; it = it->tail) {
    if (it->core) {
      if (!it->shadowed) {
#ifdef ENABLE_Z3
        it->shadowConstraint =
            (NoExistential ? it->constraint
                           : ShadowArray::getShadowExpression(it->constraint,
                                                              replacements));
#else
        it->shadowConstraint = it->constraint;
#endif
        it->shadowed = true;
        it->boundVariables.insert(replacements.begin(), replacements.end());
      } else {
        // Already shadowed, we add the bound variables
        replacements.insert(it->boundVariables.begin(),
                            it->boundVariables.end());
      }
      if (!res.isNull()) {
        res = AndExpr::create(res, it->shadowConstraint);
      } else {
        res = it->shadowConstraint;
      }
    }
  }
  return res;
}

void PathCondition::dump() const {
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void PathCondition::print(llvm::raw_ostream &stream) const {
  stream << "[";
  for (const PathCondition *it = this; it != 0; it = it->tail) {
    it->constraint->print(stream);
    stream << ": " << (it->core ? "core" : "non-core");
    if (it->tail != 0)
      stream << ",";
  }
  stream << "]";
}

/**/

Statistic SubsumptionTableEntry::concreteStoreExpressionBuildTime(
    "concreteStoreExpressionBuildTime", "concreteStoreTime");
Statistic SubsumptionTableEntry::symbolicStoreExpressionBuildTime(
    "symbolicStoreExpressionBuildTime", "symbolicStoreTime");
Statistic SubsumptionTableEntry::solverAccessTime("solverAccessTime",
                                                  "solverAccessTime");

SubsumptionTableEntry::SubsumptionTableEntry(
    TxTreeNode *node, const std::vector<llvm::Instruction *> &stack)
    : programPoint(node->getProgramPoint()),
      nodeSequenceNumber(node->getNodeSequenceNumber()) {
  existentials.clear();
  interpolant = node->getInterpolant(existentials);

  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  storedExpressions = node->getStoredCoreExpressions(stack, existentials);

  concreteAddressStore = storedExpressions.first;
  for (Dependency::ConcreteStore::iterator it = concreteAddressStore.begin(),
                                           ie = concreteAddressStore.end();
       it != ie; ++it) {
    concreteAddressStoreKeys.push_back(it->first);
  }

  symbolicAddressStore = storedExpressions.second;
  for (Dependency::SymbolicStore::iterator it = symbolicAddressStore.begin(),
                                           ie = symbolicAddressStore.end();
       it != ie; ++it) {
    symbolicAddressStoreKeys.push_back(it->first);
  }
}

SubsumptionTableEntry::~SubsumptionTableEntry() {}

bool
SubsumptionTableEntry::hasVariableInSet(std::set<const Array *> &existentials,
                                        ref<Expr> expr) {
  for (int i = 0, numKids = expr->getNumKids(); i < numKids; ++i) {
    if (llvm::isa<ReadExpr>(expr)) {
      ReadExpr *readExpr = llvm::dyn_cast<ReadExpr>(expr);
      const Array *array = (readExpr->updates).root;
      for (std::set<const Array *>::const_iterator it = existentials.begin(),
                                                   ie = existentials.end();
           it != ie; ++it) {
        if ((*it) == array)
          return true;
      }
    } else if (hasVariableInSet(existentials, expr->getKid(i)))
      return true;
  }
  return false;
}

bool SubsumptionTableEntry::hasVariableNotInSet(
    std::set<const Array *> &existentials, ref<Expr> expr) {
  for (int i = 0, numKids = expr->getNumKids(); i < numKids; ++i) {
    if (llvm::isa<ReadExpr>(expr)) {
      ReadExpr *readExpr = llvm::dyn_cast<ReadExpr>(expr);
      const Array *array = (readExpr->updates).root;
      for (std::set<const Array *>::const_iterator it = existentials.begin(),
                                                   ie = existentials.end();
           it != ie; ++it) {
        if ((*it) == array)
          return false;
      }
      return true;
    } else if (hasVariableNotInSet(existentials, expr->getKid(i)))
      return true;
  }
  return false;
}

ref<Expr>
SubsumptionTableEntry::simplifyArithmeticBody(ref<Expr> existsExpr,
                                              bool &hasExistentialsOnly) {
  ExistsExpr *expr = llvm::dyn_cast<ExistsExpr>(existsExpr);

  assert(expr && "expression is not existentially quantified");

  // Assume the we shall return general ExistsExpr that does not contain
  // only existential variables.
  hasExistentialsOnly = false;

  // We assume that the body is always a conjunction of interpolant in terms of
  // shadow (existentially-quantified) variables and state equality constraints,
  // which may contain both normal and shadow variables.
  ref<Expr> body = expr->body;

  // We only simplify a conjunction of interpolant and equalities
  if (!llvm::isa<AndExpr>(body))
    return existsExpr;

  // If the post-simplified body was a constant, simply return the body;
  if (llvm::isa<ConstantExpr>(body))
    return body;

  // The equality constraint is only a single disjunctive clause
  // of a CNF formula. In this case we simplify nothing.
  if (llvm::isa<OrExpr>(body->getKid(1)))
    return existsExpr;

  // Here we process equality constraints of shadow and normal variables.
  // The following procedure returns simplified version of the expression
  // by reducing any equality expression into constant (TRUE/FALSE).
  // if body is A and (Eq 2 4), body can be simplified into false.
  // if body is A and (Eq 2 2), body can be simplified into A.
  //
  // Along the way, It also collects the remaining equalities in equalityPack.
  // The equality constraints (body->getKid(1)) is a CNF of the form
  // c1 /\ ... /\ cn. This procedure collects into equalityPack all ci for
  // 1<=i<=n which are atomic equalities, to be used in simplifying the
  // interpolant.
  std::vector<ref<Expr> > equalityPack;

  ref<Expr> fullEqualityConstraint =
      simplifyEqualityExpr(equalityPack, body->getKid(1));

  if (fullEqualityConstraint->isFalse())
    return fullEqualityConstraint;

  // Try to simplify the interpolant. If the resulting simplification
  // was the constant true, then the equality constraints would contain
  // equality with constants only and no equality with shadow (existential)
  // variables, hence it should be safe to simply return the equality
  // constraint.
  std::vector<ref<Expr> > interpolantPack;

  ref<Expr> simplifiedInterpolant =
      simplifyInterpolantExpr(interpolantPack, body->getKid(0));
  if (simplifiedInterpolant->isTrue())
    return fullEqualityConstraint;

  if (fullEqualityConstraint->isTrue()) {
    // This is the case when the result is still an existentially-quantified
    // formula, but one that does not contain free variables.
    hasExistentialsOnly =
        !hasVariableNotInSet(expr->variables, simplifiedInterpolant);
    if (hasExistentialsOnly) {
      return existsExpr->rebuild(&simplifiedInterpolant);
    }
  }

  ref<Expr> newInterpolant;

  for (std::vector<ref<Expr> >::iterator it = interpolantPack.begin(),
                                         ie = interpolantPack.end();
       it != ie; ++it) {

    ref<Expr> interpolantAtom = (*it); // For example C cmp D

    // only process the interpolant that still has existential variables in
    // it.
    if (hasVariableInSet(expr->variables, interpolantAtom)) {
      for (std::vector<ref<Expr> >::iterator it1 = equalityPack.begin(),
                                             ie1 = equalityPack.end();
           it1 != ie1; ++it1) {

        ref<Expr> equalityConstraint =
            *it1; // For example, say this constraint is A == B
        if (equalityConstraint->isFalse()) {
          return ConstantExpr::create(0, Expr::Bool);
        } else if (equalityConstraint->isTrue()) {
          return ConstantExpr::create(1, Expr::Bool);
        }
        // Left-hand side of the equality formula (A in our example) that
        // contains
        // the shadow expression (we assume that the
        // existentially-quantified
        // shadow variable is always on the left side).
        ref<Expr> equalityConstraintLeft = equalityConstraint->getKid(0);

        // Right-hand side of the equality formula (B in our example) that
        // does
        // not contain existentially-quantified shadow variables.
        ref<Expr> equalityConstraintRight = equalityConstraint->getKid(1);

        ref<Expr> newIntpLeft;
        ref<Expr> newIntpRight;

        // When the if condition holds, we perform substitution
        if (hasSubExpression(equalityConstraintLeft,
                             interpolantAtom->getKid(0))) {
          // Here we perform substitution, where given
          // an interpolant atom and an equality constraint,
          // we try to find a subexpression in the equality constraint
          // that matches the lhs expression of the interpolant atom.

          // Here we assume that the equality constraint is A == B and the
          // interpolant atom is C cmp D.

          // newIntpLeft == B
          newIntpLeft = equalityConstraintRight;

          // If equalityConstraintLeft does not have any arithmetic
          // operation
          // we could directly assign newIntpRight = D, otherwise,
          // newIntpRight == A[D/C]
          if (!llvm::isa<BinaryExpr>(equalityConstraintLeft))
            newIntpRight = interpolantAtom->getKid(1);
          else {
            // newIntpRight is A, but with every occurrence of C replaced
            // with D
            // i.e., newIntpRight == A[D/C]
            newIntpRight =
                replaceExpr(equalityConstraintLeft, interpolantAtom->getKid(0),
                            interpolantAtom->getKid(1));
          }

          interpolantAtom = ShadowArray::createBinaryOfSameKind(
              interpolantAtom, newIntpLeft, newIntpRight);
        }
      }
    }

    // We add the modified interpolant conjunct into a conjunction of
    // new interpolants.
    if (!newInterpolant.isNull()) {
      newInterpolant = AndExpr::create(newInterpolant, interpolantAtom);
    } else {
      newInterpolant = interpolantAtom;
    }
  }

  ref<Expr> newBody;

  if (!newInterpolant.isNull()) {
    if (!hasVariableInSet(expr->variables, newInterpolant))
      return newInterpolant;

    newBody = AndExpr::create(newInterpolant, fullEqualityConstraint);
  } else {
    newBody = AndExpr::create(simplifiedInterpolant, fullEqualityConstraint);
  }

  return existsExpr->rebuild(&newBody);
}

ref<Expr> SubsumptionTableEntry::replaceExpr(ref<Expr> originalExpr,
                                             ref<Expr> replacedExpr,
                                             ref<Expr> replacementExpr) {
  // We only handle binary expressions
  if (!llvm::isa<BinaryExpr>(originalExpr) ||
      llvm::isa<ConcatExpr>(originalExpr))
    return originalExpr;

  if (originalExpr->getKid(0) == replacedExpr)
    return ShadowArray::createBinaryOfSameKind(originalExpr, replacementExpr,
                                               originalExpr->getKid(1));

  if (originalExpr->getKid(1) == replacedExpr)
    return ShadowArray::createBinaryOfSameKind(
        originalExpr, originalExpr->getKid(0), replacementExpr);

  return ShadowArray::createBinaryOfSameKind(
      originalExpr,
      replaceExpr(originalExpr->getKid(0), replacedExpr, replacementExpr),
      replaceExpr(originalExpr->getKid(1), replacedExpr, replacementExpr));
}

bool SubsumptionTableEntry::hasSubExpression(ref<Expr> expr,
                                             ref<Expr> subExpr) {
  if (expr == subExpr)
    return true;
  if (expr->getNumKids() < 2 && expr != subExpr)
    return false;

  return hasSubExpression(expr->getKid(0), subExpr) ||
         hasSubExpression(expr->getKid(1), subExpr);
}

ref<Expr> SubsumptionTableEntry::simplifyInterpolantExpr(
    std::vector<ref<Expr> > &interpolantPack, ref<Expr> expr) {
  if (expr->getNumKids() < 2)
    return expr;

  if (llvm::isa<EqExpr>(expr) && llvm::isa<ConstantExpr>(expr->getKid(0)) &&
      llvm::isa<ConstantExpr>(expr->getKid(1))) {
    return (expr->getKid(0) == expr->getKid(1))
               ? ConstantExpr::create(1, Expr::Bool)
               : ConstantExpr::create(0, Expr::Bool);
  } else if (llvm::isa<NeExpr>(expr) &&
             llvm::isa<ConstantExpr>(expr->getKid(0)) &&
             llvm::isa<ConstantExpr>(expr->getKid(1))) {
    return (expr->getKid(0) != expr->getKid(1))
               ? ConstantExpr::create(1, Expr::Bool)
               : ConstantExpr::create(0, Expr::Bool);
  }

  ref<Expr> lhs = expr->getKid(0);
  ref<Expr> rhs = expr->getKid(1);

  if (!llvm::isa<AndExpr>(expr)) {

    // If the current expression has a form like (Eq false P), where P is some
    // comparison, we change it into the negation of P.
    if (llvm::isa<EqExpr>(expr) && expr->getKid(0)->getWidth() == Expr::Bool &&
        expr->getKid(0)->isFalse()) {
      if (llvm::isa<SltExpr>(rhs)) {
        expr = SgeExpr::create(rhs->getKid(0), rhs->getKid(1));
      } else if (llvm::isa<SgeExpr>(rhs)) {
        expr = SltExpr::create(rhs->getKid(0), rhs->getKid(1));
      } else if (llvm::isa<SleExpr>(rhs)) {
        expr = SgtExpr::create(rhs->getKid(0), rhs->getKid(1));
      } else if (llvm::isa<SgtExpr>(rhs)) {
        expr = SleExpr::create(rhs->getKid(0), rhs->getKid(1));
      }
    }

    // Collect unique interpolant expressions in one vector
    if (std::find(interpolantPack.begin(), interpolantPack.end(), expr) ==
        interpolantPack.end())
      interpolantPack.push_back(expr);

    return expr;
  }

  ref<Expr> simplifiedLhs = simplifyInterpolantExpr(interpolantPack, lhs);
  if (simplifiedLhs->isFalse())
    return simplifiedLhs;

  ref<Expr> simplifiedRhs = simplifyInterpolantExpr(interpolantPack, rhs);
  if (simplifiedRhs->isFalse())
    return simplifiedRhs;

  if (simplifiedLhs->isTrue())
    return simplifiedRhs;

  if (simplifiedRhs->isTrue())
    return simplifiedLhs;

  return AndExpr::create(simplifiedLhs, simplifiedRhs);
}

ref<Expr> SubsumptionTableEntry::simplifyEqualityExpr(
    std::vector<ref<Expr> > &equalityPack, ref<Expr> expr) {
  if (expr->getNumKids() < 2)
    return expr;

  if (llvm::isa<EqExpr>(expr)) {
    if (llvm::isa<ConstantExpr>(expr->getKid(0)) &&
        llvm::isa<ConstantExpr>(expr->getKid(1))) {
      return (expr->getKid(0) == expr->getKid(1))
                 ? ConstantExpr::create(1, Expr::Bool)
                 : ConstantExpr::create(0, Expr::Bool);
    }

    // Collect unique equality and in-equality expressions in one vector
    if (std::find(equalityPack.begin(), equalityPack.end(), expr) ==
        equalityPack.end())
      equalityPack.push_back(expr);

    return expr;
  }

  if (llvm::isa<AndExpr>(expr)) {
    ref<Expr> lhs = simplifyEqualityExpr(equalityPack, expr->getKid(0));
    if (lhs->isFalse())
      return lhs;

    ref<Expr> rhs = simplifyEqualityExpr(equalityPack, expr->getKid(1));
    if (rhs->isFalse())
      return rhs;

    if (lhs->isTrue())
      return rhs;

    if (rhs->isTrue())
      return lhs;

    return AndExpr::create(lhs, rhs);
  } else if (llvm::isa<OrExpr>(expr)) {
    // We provide throw-away dummy equalityPack, as we do not use the atomic
    // equalities within disjunctive clause to simplify the interpolant.
    std::vector<ref<Expr> > dummy;
    ref<Expr> lhs = simplifyEqualityExpr(dummy, expr->getKid(0));
    if (lhs->isTrue())
      return lhs;

    ref<Expr> rhs = simplifyEqualityExpr(dummy, expr->getKid(1));
    if (rhs->isTrue())
      return rhs;

    if (lhs->isFalse())
      return rhs;

    if (rhs->isFalse())
      return lhs;

    return OrExpr::create(lhs, rhs);
  }

  if (expr->getWidth() == Expr::Bool)
    return expr;

  assert(!"Invalid expression type.");
}

ref<Expr>
SubsumptionTableEntry::getSubstitution(ref<Expr> equalities,
                                       std::map<ref<Expr>, ref<Expr> > &map) {
  // It is assumed the lhs is an expression on the existentially-quantified
  // variable whereas the rhs is an expression on the free variables.
  if (llvm::isa<EqExpr>(equalities)) {
    ref<Expr> lhs = equalities->getKid(0);
    if (isVariable(lhs)) {
      map[lhs] = equalities->getKid(1);
      return ConstantExpr::create(1, Expr::Bool);
    }
    return equalities;
  }

  if (llvm::isa<AndExpr>(equalities)) {
    ref<Expr> lhs = getSubstitution(equalities->getKid(0), map);
    ref<Expr> rhs = getSubstitution(equalities->getKid(1), map);
    if (lhs->isTrue()) {
      if (rhs->isTrue()) {
        return ConstantExpr::create(1, Expr::Bool);
      }
      return rhs;
    } else {
      if (rhs->isTrue()) {
        return lhs;
      }
      return AndExpr::create(lhs, rhs);
    }
  }
  return equalities;
}

bool SubsumptionTableEntry::detectConflictPrimitives(ExecutionState &state,
                                                     ref<Expr> query) {
  if (llvm::isa<ExistsExpr>(query))
    return true;

  std::vector<ref<Expr> > conjunction;
  if (!fetchQueryEqualityConjuncts(conjunction, query)) {
    return false;
  }

  for (std::vector<ref<Expr> >::const_iterator it1 = state.constraints.begin(),
                                               ie1 = state.constraints.end();
       it1 != ie1; ++it1) {

    if ((*it1)->getKind() != Expr::Eq)
      continue;

    for (std::vector<ref<Expr> >::const_iterator it2 = conjunction.begin(),
                                                 ie2 = conjunction.end();
         it2 != ie2; ++it2) {

      ref<Expr> stateConstraintExpr = it1->get();
      ref<Expr> queryExpr = it2->get();

      if (stateConstraintExpr != queryExpr &&
          (llvm::isa<EqExpr>(stateConstraintExpr) ||
           llvm::isa<EqExpr>(queryExpr))) {

        if (stateConstraintExpr ==
                EqExpr::create(ConstantExpr::create(0, Expr::Bool),
                               queryExpr) ||
            EqExpr::create(ConstantExpr::create(0, Expr::Bool),
                           stateConstraintExpr) == queryExpr) {
          return false;
        }
      }
    }
  }

  return true;
}

bool SubsumptionTableEntry::fetchQueryEqualityConjuncts(
    std::vector<ref<Expr> > &conjunction, ref<Expr> query) {

  if (!llvm::isa<AndExpr>(query)) {
    if (query->getKind() == Expr::Eq) {

      EqExpr *equality = llvm::dyn_cast<EqExpr>(query);

      if (llvm::isa<ConstantExpr>(equality->getKid(0)) &&
          llvm::isa<ConstantExpr>(equality->getKid(1)) &&
          equality->getKid(0) != equality->getKid(1)) {
        return false;
      } else {
        conjunction.push_back(query);
      }
    }
    return true;
  }

  return fetchQueryEqualityConjuncts(conjunction, query->getKid(0)) &&
         fetchQueryEqualityConjuncts(conjunction, query->getKid(1));
}

ref<Expr> SubsumptionTableEntry::simplifyExistsExpr(ref<Expr> existsExpr,
                                                    bool &hasExistentialsOnly) {
  assert(llvm::isa<ExistsExpr>(existsExpr));

  ref<Expr> body = llvm::dyn_cast<ExistsExpr>(existsExpr)->body;
  assert(llvm::isa<AndExpr>(body));

  std::map<ref<Expr>, ref<Expr> > substitution;
  ref<Expr> equalities = getSubstitution(body->getKid(1), substitution);
  ref<Expr> interpolant =
      ApplySubstitutionVisitor(substitution).visit(body->getKid(0));

  ExistsExpr *expr = llvm::dyn_cast<ExistsExpr>(existsExpr);
  if (hasVariableInSet(expr->variables, equalities)) {
    // we could also replace the occurrence of some variables with its
    // corresponding substitution mapping.
    equalities = ApplySubstitutionVisitor(substitution).visit(equalities);
  }

  ref<Expr> newBody = AndExpr::create(interpolant, equalities);

  // FIXME: Need to test the performance of the following.
  if (!hasVariableInSet(expr->variables, newBody))
    return newBody;

  ref<Expr> ret = simplifyArithmeticBody(existsExpr->rebuild(&newBody),
                                         hasExistentialsOnly);
  return ret;
}

bool SubsumptionTableEntry::subsumed(
    TimingSolver *solver, ExecutionState &state, double timeout,
    const std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
        storedExpressions, int debugLevel) {
#ifdef ENABLE_Z3
  // Tell the solver implementation that we are checking for subsumption for
  // collecting statistics of solver calls.
  SubsumptionCheckMarker subsumptionCheckMarker;

  // Quick check for subsumption in case the interpolant is empty
  if (empty()) {
    if (debugLevel >= 1) {
      klee_message("#%lu=>#%lu: Check success due to empty table entry",
                   state.txTreeNode->getNodeSequenceNumber(),
                   nodeSequenceNumber);
    }
    return true;
  }

  Dependency::ConcreteStore stateConcreteAddressStore = storedExpressions.first;

  Dependency::SymbolicStore stateSymbolicAddressStore =
      storedExpressions.second;

  ref<Expr> stateEqualityConstraints;

  std::map<ref<StoredValue>, std::set<ref<Expr> > >
  corePointerValues; // Pointer values in the core for memory bounds
  // interpolation

  {
    TimerStatIncrementer t(concreteStoreExpressionBuildTime);

    // Build constraints from concrete-address interpolant store
    for (std::vector<llvm::Value *>::iterator
             it1 = concreteAddressStoreKeys.begin(),
             ie1 = concreteAddressStoreKeys.end();
         it1 != ie1; ++it1) {

      const Dependency::ConcreteStoreMap tabledConcreteMap =
          concreteAddressStore[*it1];
      const Dependency::ConcreteStoreMap stateConcreteMap =
          stateConcreteAddressStore[*it1];
      const Dependency::SymbolicStoreMap stateSymbolicMap =
          stateSymbolicAddressStore[*it1];

      // If the current state does not constrain the same base, subsumption
      // fails.
      if (stateConcreteMap.empty() && stateSymbolicMap.empty()) {
        if (debugLevel >= 1) {
          klee_message("#%lu=>#%lu: Check failure due to empty state concrete "
                       "and symbolic maps",
                       state.txTreeNode->getNodeSequenceNumber(),
                       nodeSequenceNumber);
        }
        return false;
      }

      for (Dependency::ConcreteStoreMap::const_iterator
               it2 = tabledConcreteMap.begin(),
               ie2 = tabledConcreteMap.end();
           it2 != ie2; ++it2) {

        // The address is not constrained by the current state, therefore
        // the current state is incomparable to the stored interpolant,
        // and we therefore fail the subsumption.
        if (!stateConcreteMap.count(it2->first)) {
          if (debugLevel >= 1) {
            klee_message("#%lu=>#%lu: Check failure as memory region in the "
                         "table does not "
                         "exist in the state",
                         state.txTreeNode->getNodeSequenceNumber(),
                         nodeSequenceNumber);
          }
          return false;
        }

        const ref<StoredValue> tabledValue = it2->second;
        const ref<StoredValue> stateValue = stateConcreteMap.at(it2->first);
        ref<Expr> res;

        if (!stateValue.isNull()) {
          // There is the corresponding concrete allocation
          if (tabledValue->getExpression()->getWidth() !=
              stateValue->getExpression()->getWidth()) {
            // We conservatively fail the subsumption in case the sizes do not
            // match.
            if (debugLevel >= 1) {
              klee_message("#%lu=>#%lu: Check failure as sizes of stored "
                           "values do not match",
                           state.txTreeNode->getNodeSequenceNumber(),
                           nodeSequenceNumber);
            }
            return false;
          } else if (!NoBoundInterpolation && !ExactAddressInterpolant &&
                     tabledValue->isPointer() && stateValue->isPointer() &&
                     tabledValue->useBound()) {
            std::set<ref<Expr> > bounds;
            ref<Expr> boundsCheck =
                tabledValue->getBoundsCheck(stateValue, bounds, debugLevel);
            if (boundsCheck->isFalse()) {
              if (debugLevel >= 1) {
                klee_message("#%lu=>#%lu: Check failure due to failure in "
                             "memory bounds check",
                             state.txTreeNode->getNodeSequenceNumber(),
                             nodeSequenceNumber);
              }
              return false;
            }
            if (!boundsCheck->isTrue())
              res = boundsCheck;

            // We record the LLVM value of the pointer
            corePointerValues[stateValue] = bounds;
          } else {
            res = EqExpr::create(tabledValue->getExpression(),
                                 stateValue->getExpression());
            if (res->isFalse()) {
              if (debugLevel >= 1) {
                std::string msg;
                llvm::raw_string_ostream stream(msg);
                tabledValue->getExpression()->print(stream);
                stream << " vs. ";
                stateValue->getExpression()->print(stream);
                stream.flush();
                klee_message(
                    "#%lu=>#%lu: Check failure due to unequal content: %s",
                    state.txTreeNode->getNodeSequenceNumber(),
                    nodeSequenceNumber, msg.c_str());
              }
              return false;
            }
          }
        }

        if (!stateSymbolicMap.empty()) {
          const ref<Expr> tabledConcreteOffset = it2->first->getOffset();
          ref<Expr> conjunction;

          for (Dependency::SymbolicStoreMap::const_iterator
                   it3 = stateSymbolicMap.begin(),
                   ie3 = stateSymbolicMap.end();
               it3 != ie3; ++it3) {

            // We make sure the context part of the addresses (the allocation
            // site and the call stack) are equivalent.
            if (it2->first->compareContext(it3->first->getValue(),
                                           it3->first->getStack()))
              continue;

            ref<Expr> stateSymbolicOffset = it3->first->getOffset();
            ref<StoredValue> stateSymbolicValue = it3->second;
            ref<Expr> newTerm;

            if (tabledValue->getExpression()->getWidth() !=
                stateSymbolicValue->getExpression()->getWidth()) {
              // We conservatively require that the addresses should not be
              // equal whenever their values are of different width
              newTerm = EqExpr::create(
                  ConstantExpr::create(0, Expr::Bool),
                  EqExpr::create(tabledConcreteOffset, stateSymbolicOffset));

            } else if (!NoBoundInterpolation && !ExactAddressInterpolant &&
                       tabledValue->isPointer() &&
                       stateSymbolicValue->isPointer() &&
                       tabledValue->useBound()) {
              std::set<ref<Expr> > bounds;
              ref<Expr> boundsCheck = tabledValue->getBoundsCheck(
                  stateSymbolicValue, bounds, debugLevel);

              if (!boundsCheck->isTrue()) {
                newTerm = EqExpr::create(
                    ConstantExpr::create(0, Expr::Bool),
                    EqExpr::create(tabledConcreteOffset, stateSymbolicOffset));

                if (!boundsCheck->isFalse()) {
                  // Implication: if tabledConcreteAddress ==
                  // stateSymbolicAddress
                  // then bounds check must hold
                  newTerm = OrExpr::create(newTerm, boundsCheck);
                }
              }

              // We record the LLVM value of the pointer
              corePointerValues[stateValue] = bounds;
            } else {
              // Implication: if tabledConcreteAddress == stateSymbolicAddress,
              // then tabledValue->getExpression() ==
              // stateSymbolicValue->getExpression()
              newTerm = OrExpr::create(
                  EqExpr::create(ConstantExpr::create(0, Expr::Bool),
                                 EqExpr::create(tabledConcreteOffset,
                                                stateSymbolicOffset)),
                  EqExpr::create(tabledValue->getExpression(),
                                 stateSymbolicValue->getExpression()));
            }
            if (!conjunction.isNull()) {
              conjunction = AndExpr::create(newTerm, conjunction);
            } else {
              // Implication: if tabledConcreteAddress == stateSymbolicAddress,
              // then tabledValue == stateSymbolicValue
              newTerm = OrExpr::create(
                  EqExpr::create(ConstantExpr::create(0, Expr::Bool),
                                 EqExpr::create(tabledConcreteOffset,
                                                stateSymbolicOffset)),
                  EqExpr::create(tabledValue->getExpression(),
                                 stateSymbolicValue->getExpression()));
            }

            if (!newTerm.isNull()) {
              if (!conjunction.isNull()) {
                conjunction = AndExpr::create(newTerm, conjunction);
              } else {
                conjunction = newTerm;
              }
            }
          }
          // If there were corresponding concrete as well as symbolic
          // allocations in the current state, conjunct them
          if (!conjunction.isNull()) {
            res = (!res.isNull() ? AndExpr::create(res, conjunction)
                                 : conjunction);
          }
        }

        if (!res.isNull()) {
          stateEqualityConstraints =
              (stateEqualityConstraints.isNull()
                   ? res
                   : AndExpr::create(res, stateEqualityConstraints));
        }
      }
    }
  }

  {
    TimerStatIncrementer t(symbolicStoreExpressionBuildTime);
    // Build constraints from symbolic-address interpolant store
    for (std::vector<llvm::Value *>::iterator
             it1 = symbolicAddressStoreKeys.begin(),
             ie1 = symbolicAddressStoreKeys.end();
         it1 != ie1; ++it1) {
      const Dependency::SymbolicStoreMap tabledSymbolicMap =
          symbolicAddressStore[*it1];
      const Dependency::ConcreteStoreMap stateConcreteMap =
          stateConcreteAddressStore[*it1];
      const Dependency::SymbolicStoreMap stateSymbolicMap =
          stateSymbolicAddressStore[*it1];

      ref<Expr> conjunction;

      for (Dependency::SymbolicStoreMap::const_iterator
               it2 = tabledSymbolicMap.begin(),
               ie2 = tabledSymbolicMap.end();
           it2 != ie2; ++it2) {
        ref<Expr> tabledSymbolicOffset = it2->first->getOffset();
        ref<StoredValue> tabledValue = it2->second;

        for (Dependency::ConcreteStoreMap::const_iterator
                 it3 = stateConcreteMap.begin(),
                 ie3 = stateConcreteMap.end();
             it3 != ie3; ++it3) {

          // We make sure the context part of the addresses (the allocation
          // site and the call stack) are equivalent.
          if (it2->first->compareContext(it3->first->getValue(),
                                         it3->first->getStack()))
            continue;

          ref<Expr> stateConcreteOffset = it3->first->getOffset();
          ref<StoredValue> stateValue = it3->second;
          ref<Expr> newTerm;

          if (tabledValue->getExpression()->getWidth() !=
              stateValue->getExpression()->getWidth()) {
            // We conservatively require that the addresses should not be equal
            // whenever their values are of different width
            newTerm = EqExpr::create(
                ConstantExpr::create(0, Expr::Bool),
                EqExpr::create(tabledSymbolicOffset, stateConcreteOffset));
          } else if (!NoBoundInterpolation && !ExactAddressInterpolant &&
                     tabledValue->isPointer() && stateValue->isPointer() &&
                     tabledValue->useBound()) {
            std::set<ref<Expr> > bounds;
            ref<Expr> boundsCheck =
                tabledValue->getBoundsCheck(stateValue, bounds, debugLevel);

            if (!boundsCheck->isTrue()) {
              newTerm = EqExpr::create(
                  ConstantExpr::create(0, Expr::Bool),
                  EqExpr::create(tabledSymbolicOffset, stateConcreteOffset));

              if (!boundsCheck->isFalse()) {
                // Implication: if tabledConcreteAddress == stateSymbolicAddress
                // then bounds check must hold
                newTerm = OrExpr::create(newTerm, boundsCheck);
              }
            }

            // We record the LLVM value of the pointer
            corePointerValues[stateValue] = bounds;
          } else {
            // Implication: if tabledSymbolicAddress == stateConcreteAddress,
            // then tabledValue == stateValue
            newTerm = OrExpr::create(
                EqExpr::create(
                    ConstantExpr::create(0, Expr::Bool),
                    EqExpr::create(tabledSymbolicOffset, stateConcreteOffset)),
                EqExpr::create(tabledValue->getExpression(),
                               stateValue->getExpression()));
          }

          if (!newTerm.isNull()) {
            if (!conjunction.isNull()) {
              conjunction = AndExpr::create(newTerm, conjunction);
            } else {
              conjunction = newTerm;
            }
          }
        }

        for (Dependency::SymbolicStoreMap::const_iterator
                 it3 = stateSymbolicMap.begin(),
                 ie3 = stateSymbolicMap.end();
             it3 != ie3; ++it3) {

          // We make sure the context part of the addresses (the allocation
          // site and the call stack) are equivalent.
          if (it2->first->compareContext(it3->first->getValue(),
                                         it3->first->getStack()))
            continue;

          ref<Expr> stateSymbolicOffset = it3->first->getOffset();
          ref<StoredValue> stateValue = it3->second;
          ref<Expr> newTerm;

          if (tabledValue->getExpression()->getWidth() !=
              stateValue->getExpression()->getWidth()) {
            // We conservatively require that the addresses should not be equal
            // whenever their values are of different width
            newTerm = EqExpr::create(
                ConstantExpr::create(0, Expr::Bool),
                EqExpr::create(tabledSymbolicOffset, stateSymbolicOffset));
          } else if (!NoBoundInterpolation && !ExactAddressInterpolant &&
                     tabledValue->isPointer() && stateValue->isPointer() &&
                     tabledValue->useBound()) {
            std::set<ref<Expr> > bounds;
            ref<Expr> boundsCheck =
                tabledValue->getBoundsCheck(stateValue, bounds, debugLevel);

            if (!boundsCheck->isTrue()) {
              newTerm = EqExpr::create(
                  ConstantExpr::create(0, Expr::Bool),
                  EqExpr::create(tabledSymbolicOffset, stateSymbolicOffset));

              if (!boundsCheck->isFalse()) {
                // Implication: if tabledConcreteAddress == stateSymbolicAddress
                // then bounds check must hold
                newTerm = OrExpr::create(newTerm, boundsCheck);
              }
            }

            // We record the LLVM value of the pointer
            corePointerValues[stateValue] = bounds;
          } else {
            // Implication: if tabledSymbolicAddress == stateSymbolicAddress
            // then tabledValue == stateValue
            newTerm = OrExpr::create(
                EqExpr::create(
                    ConstantExpr::create(0, Expr::Bool),
                    EqExpr::create(tabledSymbolicOffset, stateSymbolicOffset)),
                EqExpr::create(tabledValue->getExpression(),
                               stateValue->getExpression()));
          }

          if (!newTerm.isNull()) {
            if (!conjunction.isNull()) {
              conjunction = AndExpr::create(newTerm, conjunction);
            } else {
              conjunction = newTerm;
            }
          }
        }
      }

      if (!conjunction.isNull()) {
        stateEqualityConstraints =
            (stateEqualityConstraints.isNull()
                 ? conjunction
                 : AndExpr::create(conjunction, stateEqualityConstraints));
      }
    }
  }

  Solver::Validity result;
  ref<Expr> query;

  {
    TimerStatIncrementer t(solverAccessTime);

    // Here we build the query, after which it is always a conjunction of
    // the interpolant and the state equality constraints. Here we call
    // AndExpr::alloc instead of AndExpr::create as we need to guarantee that
    // the resulting expression is an AndExpr, otherwise simplifyExistsExpr
    // would not work.
    if (!interpolant.isNull()) {
      query = !stateEqualityConstraints.isNull()
                  ? AndExpr::alloc(interpolant, stateEqualityConstraints)
                  : AndExpr::alloc(interpolant,
                                   ConstantExpr::create(1, Expr::Bool));
    } else if (!stateEqualityConstraints.isNull()) {
      query = AndExpr::alloc(ConstantExpr::create(1, Expr::Bool),
                             stateEqualityConstraints);
    } else {
      // Here both the interpolant constraints and state equality
      // constraints are empty, therefore everything gets subsumed
      if (debugLevel >= 1) {
        klee_message("#%lu=>#%lu: Check success as interpolant is empty",
                     state.txTreeNode->getNodeSequenceNumber(),
                     nodeSequenceNumber);
      }

      // We build memory bounds interpolants from pointer values
      std::string reason = "";
      if (debugLevel >= 1) {
        llvm::raw_string_ostream stream(reason);
        llvm::Instruction *instr = state.pc->inst;
        stream << "interpolating memory bound for subsumption at ";
        if (instr->getParent()->getParent()) {
          std::string functionName(
              instr->getParent()->getParent()->getName().str());
          stream << functionName << ": ";
          if (llvm::MDNode *n = instr->getMetadata("dbg")) {
            llvm::DILocation loc(n);
            stream << "Line " << loc.getLineNumber();
          } else {
            instr->print(stream);
          }
        } else {
          instr->print(stream);
        }
      }
      for (std::map<ref<StoredValue>, std::set<ref<Expr> > >::iterator
               it = corePointerValues.begin(),
               ie = corePointerValues.end();
           it != ie; ++it) {
        state.txTreeNode->pointerValuesInterpolation(it->first->getValue(),
                                                     it->first->getExpression(),
                                                     it->second, reason);
      }
      return true;
    }

    bool queryHasNoFreeVariables = false;

    if (!existentials.empty()) {
      ref<Expr> existsExpr = ExistsExpr::create(existentials, query);
      if (debugLevel >= 2) {
        klee_message("Before simplification:\n%s",
                     PrettyExpressionBuilder::constructQuery(
                         state.constraints, existsExpr).c_str());
      }
      query = simplifyExistsExpr(existsExpr, queryHasNoFreeVariables);
    }

    // If query simplification result was false, we quickly fail without calling
    // the solver
    if (query->isFalse()) {
      if (debugLevel >= 1) {
        klee_message("#%lu=>#%lu: Check failure as consequent is unsatisfiable",
                     state.txTreeNode->getNodeSequenceNumber(),
                     nodeSequenceNumber);
      }
      return false;
    }

    bool success = false;

    if (!detectConflictPrimitives(state, query)) {
      if (debugLevel >= 1) {
        klee_message(
            "#%lu=>#%lu: Check failure as contradictory equalities detected",
            state.txTreeNode->getNodeSequenceNumber(), nodeSequenceNumber);
      }
      return false;
    }

    Z3Solver *z3solver = 0;

    // We call the solver only when the simplified query is not a constant and
    // no contradictory unary constraints found from solvingUnaryConstraints
    // method.
    if (!llvm::isa<ConstantExpr>(query)) {
      if (!existentials.empty() && llvm::isa<ExistsExpr>(query)) {
        if (debugLevel >= 2) {
          klee_message("Existentials not empty");
        }

        // Instantiate a new Z3 solver to make sure we use Z3
        // without pre-solving optimizations. It would be nice
        // in the future to just run solver->evaluate so that
        // the optimizations can be used, but this requires
        // handling of quantified expressions by KLEE's pre-solving
        // procedure, which does not exist currently.
        z3solver = new Z3Solver();

        z3solver->setCoreSolverTimeout(timeout);

        if (queryHasNoFreeVariables) {
          // In case the query has no free variables, we reformulate
          // the solver call as satisfiability check of the body of
          // the query.
          ConstraintManager constraints;
          ref<ConstantExpr> tmpExpr;

          ref<Expr> falseExpr = ConstantExpr::create(0, Expr::Bool);
          constraints.addConstraint(
              EqExpr::create(falseExpr, query->getKid(0)));

          if (debugLevel >= 2) {
            klee_message("Querying for satisfiability check:\n%s",
                         PrettyExpressionBuilder::constructQuery(
                             constraints, falseExpr).c_str());
          }

          success = z3solver->getValue(Query(constraints, falseExpr), tmpExpr);
          result = success ? Solver::True : Solver::Unknown;
        } else {
          if (debugLevel >= 2) {
            klee_message("Querying for subsumption check:\n%s",
                         PrettyExpressionBuilder::constructQuery(
                             state.constraints, query).c_str());
          }

          success = z3solver->directComputeValidity(
              Query(state.constraints, query), result);
        }

        z3solver->setCoreSolverTimeout(0);

      } else {
        if (debugLevel >= 2) {
          klee_message("Querying for subsumption check:\n%s",
                       PrettyExpressionBuilder::constructQuery(
                           state.constraints, query).c_str());
        }
        // We call the solver in the standard way if the
        // formula is unquantified.
        solver->setTimeout(timeout);
        success = solver->evaluate(state, query, result);
        solver->setTimeout(0);
      }
    } else {
      // query is a constant expression
      if (query->isTrue()) {
        if (debugLevel >= 1) {
          klee_message("#%lu=>#%lu: Check success as query is true",
                       state.txTreeNode->getNodeSequenceNumber(),
                       nodeSequenceNumber);
        }

        if (!NoBoundInterpolation && !ExactAddressInterpolant) {
          // We build memory bounds interpolants from pointer values
          std::string reason = "";
          if (debugLevel >= 1) {
            llvm::raw_string_ostream stream(reason);
            llvm::Instruction *instr = state.pc->inst;
            stream << "interpolating memory bound for subsumption at ";
            if (instr->getParent()->getParent()) {
              std::string functionName(
                  instr->getParent()->getParent()->getName().str());
              stream << functionName << ": ";
              if (llvm::MDNode *n = instr->getMetadata("dbg")) {
                llvm::DILocation loc(n);
                stream << "Line " << loc.getLineNumber();
              } else {
                instr->print(stream);
              }
            } else {
              instr->print(stream);
            }
          }
          for (std::map<ref<StoredValue>, std::set<ref<Expr> > >::iterator
                   it = corePointerValues.begin(),
                   ie = corePointerValues.end();
               it != ie; ++it) {
            state.txTreeNode->pointerValuesInterpolation(
                it->first->getValue(), it->first->getExpression(), it->second,
                reason);
          }
        }
        return true;
      }
      if (debugLevel >= 1) {
        klee_message("#%lu=>#%lu: Check failure as query is non-true",
                     state.txTreeNode->getNodeSequenceNumber(),
                     nodeSequenceNumber);
      }
      return false;
    }

    if (success && result == Solver::True) {
      const std::vector<ref<Expr> > &unsatCore =
          (z3solver ? z3solver->getUnsatCore() : solver->getUnsatCore());

      // State subsumed, we mark needed constraints on the
      // path condition.
      if (debugLevel >= 1) {
        klee_message("#%lu=>#%lu: Check success as solver decided validity",
                     state.txTreeNode->getNodeSequenceNumber(),
                     nodeSequenceNumber);
      }

      // We create path condition marking structure and mark core constraints
      state.txTreeNode->unsatCoreInterpolation(unsatCore);

      if (!NoBoundInterpolation && !ExactAddressInterpolant) {
        // We build memory bounds interpolants from pointer values
        std::string reason = "";
        if (debugLevel >= 1) {
          llvm::raw_string_ostream stream(reason);
          llvm::Instruction *instr = state.pc->inst;
          stream << "interpolating memory bound for subsumption at ";
          if (instr->getParent()->getParent()) {
            std::string functionName(
                instr->getParent()->getParent()->getName().str());
            stream << functionName << ": ";
            if (llvm::MDNode *n = instr->getMetadata("dbg")) {
              llvm::DILocation loc(n);
              stream << "Line " << loc.getLineNumber();
            } else {
              instr->print(stream);
            }
          } else {
            instr->print(stream);
          }
        }
        for (std::map<ref<StoredValue>, std::set<ref<Expr> > >::iterator
                 it = corePointerValues.begin(),
                 ie = corePointerValues.end();
             it != ie; ++it) {
          state.txTreeNode->pointerValuesInterpolation(
              it->first->getValue(), it->first->getExpression(), it->second,
              reason);
        }
      }

      return true;
    }

    // Here the solver could not decide that the subsumption is valid. It may
    // have decided invalidity, however, CexCachingSolver::computeValidity,
    // which was eventually called from solver->evaluate is conservative, where
    // it returns Solver::Unknown even in case when invalidity is established by
    // the solver.
    if (z3solver)
      delete z3solver;

    if (debugLevel >= 1) {
      klee_message(
          "#%lu=>#%lu: Check failure as solver did not decide validity",
          state.txTreeNode->getNodeSequenceNumber(), nodeSequenceNumber);
    }
  }
#endif /* ENABLE_Z3 */
  return false;
}

ref<Expr> SubsumptionTableEntry::getInterpolant() const { return interpolant; }

void SubsumptionTableEntry::print(llvm::raw_ostream &stream) const {
  print(stream, 0);
}

void SubsumptionTableEntry::print(llvm::raw_ostream &stream,
                                  const unsigned paddingAmount) const {
  print(stream, makeTabs(paddingAmount));
}

void SubsumptionTableEntry::print(llvm::raw_ostream &stream,
                                  const std::string &prefix) const {
  std::string tabsNext = appendTab(prefix);
  std::string tabsNextNext = appendTab(tabsNext);

  stream << prefix << "------------ Subsumption Table Entry ------------\n";
  stream << prefix << "Program point = " << programPoint << "\n";
  stream << prefix << "interpolant = ";
  if (!interpolant.isNull())
    interpolant->print(stream);
  else
    stream << "(empty)";
  stream << "\n";

  if (!concreteAddressStore.empty()) {
    stream << prefix << "concrete store = [\n";
    for (Dependency::ConcreteStore::const_iterator
             is1 = concreteAddressStore.begin(),
             ie1 = concreteAddressStore.end(), it1 = is1;
         it1 != ie1; ++it1) {
      for (Dependency::ConcreteStoreMap::const_iterator
               is2 = it1->second.begin(),
               ie2 = it1->second.end(), it2 = is2;
           it2 != ie2; ++it2) {
        if (it1 != is1 || it2 != is2)
          stream << tabsNext << "------------------------------------------\n";
        stream << tabsNext << "address:\n";
        it2->first->print(stream, tabsNextNext);
        stream << "\n";
        stream << tabsNext << "content:\n";
        it2->second->print(stream, tabsNextNext);
        stream << "\n";
      }
    }
    stream << prefix << "]\n";
  }

  if (!symbolicAddressStore.empty()) {
    stream << prefix << "symbolic store = [\n";
    for (Dependency::SymbolicStore::const_iterator
             is1 = symbolicAddressStore.begin(),
             ie1 = symbolicAddressStore.end(), it1 = is1;
         it1 != ie1; ++it1) {
      for (Dependency::SymbolicStoreMap::const_iterator
               is2 = it1->second.begin(),
               ie2 = it1->second.end(), it2 = is2;
           it2 != ie2; ++it2) {
        if (it1 != is1 || it2 != is2)
          stream << tabsNext << "------------------------------------------\n";
        stream << tabsNext << "address:\n";
        it2->first->print(stream, tabsNextNext);
        stream << "\n";
        stream << tabsNext << "content:\n";
        it2->second->print(stream, tabsNextNext);
        stream << "\n";
      }
    }
    stream << "]\n";
  }

  if (!existentials.empty()) {
    stream << prefix << "existentials = [";
    for (std::set<const Array *>::const_iterator is = existentials.begin(),
                                                 ie = existentials.end(),
                                                 it = is;
         it != ie; ++it) {
      if (it != is)
        stream << ", ";
      stream << (*it)->name;
    }
    stream << "]\n";
  }
}

void SubsumptionTableEntry::printStat(std::stringstream &stream) {
  stream << "KLEE: done:     Time for actual solver calls in subsumption check "
            "(ms) = " << ((double)stats::subsumptionQueryTime.getValue()) / 1000
         << "\n";
  stream << "KLEE: done:     Number of solver calls for subsumption check "
            "(failed) = " << stats::subsumptionQueryCount.getValue() << " ("
         << stats::subsumptionQueryFailureCount.getValue() << ")\n";
  stream << "KLEE: done:     Concrete store expression build time (ms) = "
         << ((double)concreteStoreExpressionBuildTime.getValue()) / 1000
         << "\n";
  stream << "KLEE: done:     Symbolic store expression build time (ms) = "
         << ((double)symbolicStoreExpressionBuildTime.getValue()) / 1000
         << "\n";
  stream << "KLEE: done:     Solver access time (ms) = "
         << ((double)solverAccessTime.getValue()) / 1000 << "\n";
}

/**/

void SubsumptionTable::StackIndexedTable::Node::print(llvm::raw_ostream &stream)
    const {
  print(stream, 0);
}

void SubsumptionTable::StackIndexedTable::Node::print(
    llvm::raw_ostream &stream, const unsigned paddingAmount) const {
  print(stream, makeTabs(paddingAmount));
}

void SubsumptionTable::StackIndexedTable::Node::print(
    llvm::raw_ostream &stream, const std::string &prefix) const {
  std::string tabsNext = appendTab(prefix);

  stream << "\n";
  stream << tabsNext << "Stack-indexed table tree node\n";
  if (id) {
    stream << tabsNext << "node Id = " << reinterpret_cast<uintptr_t>(id)
           << "\n";
  } else {
    stream << tabsNext << "node Id = (root)\n";
  }
  stream << tabsNext << "Entries:\n";
  for (EntryIterator it = entryList.rbegin(), ie = entryList.rend(); it != ie;
       ++it) {
    (*it)->print(stream, tabsNext);
    stream << "\n";
  }
  stream << tabsNext << "Left:\n";
  if (!left) {
    stream << tabsNext << "NULL\n";
  } else {
    left->print(stream, appendTab(prefix));
    stream << "\n";
  }
  stream << tabsNext << "Right:\n";
  if (!right) {
    stream << tabsNext << "NULL\n";
  } else {
    right->print(stream, appendTab(prefix));
    stream << "\n";
  }
}

/**/

void SubsumptionTable::StackIndexedTable::clearTree(Node *node) {
  if (node->left)
    clearTree(node->left);
  if (node->right)
    clearTree(node->right);
  delete node->left;
  delete node->right;

  for (std::deque<SubsumptionTableEntry *>::iterator
           it = node->entryList.begin(),
           ie = node->entryList.end();
       it != ie; ++it) {
    delete (*it);
  }
}

void SubsumptionTable::StackIndexedTable::insert(
    const std::vector<llvm::Instruction *> &stack,
    SubsumptionTableEntry *entry) {
  Node *current = root;

  for (std::vector<llvm::Instruction *>::const_iterator it = stack.begin(),
                                                        ie = stack.end();
       it != ie; ++it) {
    llvm::Instruction *call = *it;
    if (call < current->id) {
      if (!current->left) {
        current->left = new Node(call);
      }
      current = current->left;
      continue;
    } else if (call > current->id) {
      if (!current->right) {
        current->right = new Node(call);
      }
      current = current->right;
      continue;
    }
    break;
  }
  current->entryList.push_back(entry);
}

std::pair<SubsumptionTable::EntryIterator, SubsumptionTable::EntryIterator>
SubsumptionTable::StackIndexedTable::find(
    const std::vector<llvm::Instruction *> &stack, bool &found) const {
  Node *current = root;
  std::pair<EntryIterator, EntryIterator> ret;

  for (std::vector<llvm::Instruction *>::const_iterator it = stack.begin(),
                                                        ie = stack.end();
       it != ie; ++it) {
    llvm::Instruction *call = *it;
    if (call < current->id) {
      if (!current->left) {
        found = false;
        return ret;
      }
      current = current->left;
      continue;
    } else if (call > current->id) {
      if (!current->right) {
        found = false;
        return ret;
      }
      current = current->right;
      continue;
    }
    break;
  }
  found = true;
  return std::pair<EntryIterator, EntryIterator>(current->entryList.rbegin(),
                                                 current->entryList.rend());
}

void SubsumptionTable::StackIndexedTable::printNode(llvm::raw_ostream &stream,
                                                    Node *n,
                                                    std::string edges) const {
  if (n->left != 0) {
    stream << "\n";
    stream << edges << "+-- L:";
    n->left->print(stream, edges + "    ");
    stream << "\n";
    if (n->right != 0) {
      printNode(stream, n->left, edges + "|   ");
    } else {
      printNode(stream, n->left, edges + "    ");
    }
  }
  if (n->right != 0) {
    stream << "\n";
    stream << edges << "+-- R:";
    n->right->print(stream, edges + "    ");
    stream << "\n";
    printNode(stream, n->right, edges + "    ");
  }
}

void
SubsumptionTable::StackIndexedTable::print(llvm::raw_ostream &stream) const {
  root->print(stream);
  printNode(stream, root, "");
}

/**/

std::map<uintptr_t, SubsumptionTable::StackIndexedTable *>
SubsumptionTable::instance;

void SubsumptionTable::insert(uintptr_t id,
                              const std::vector<llvm::Instruction *> &stack,
                              SubsumptionTableEntry *entry) {
  StackIndexedTable *subTable = 0;

  TxTree::entryNumber++; // Count of entries in the table

  std::map<uintptr_t, StackIndexedTable *>::iterator it = instance.find(id);

  if (it == instance.end()) {
    subTable = new StackIndexedTable();
    subTable->insert(stack, entry);
    instance[id] = subTable;
    return;
  }
  subTable = it->second;
  subTable->insert(stack, entry);
}

bool SubsumptionTable::check(TimingSolver *solver, ExecutionState &state,
                             double timeout, int debugLevel) {
  StackIndexedTable *subTable = 0;
  TxTreeNode *txTreeNode = state.txTreeNode;

  std::map<uintptr_t, StackIndexedTable *>::iterator it =
      instance.find(state.txTreeNode->getProgramPoint());
  if (it == instance.end()) {
    if (debugLevel >= 1) {
      klee_message(
          "#%lu: Check failure due to control point not found in table",
          state.txTreeNode->getNodeSequenceNumber());
    }
    return false;
  }
  subTable = it->second;

  bool found;
  std::pair<EntryIterator, EntryIterator> iterPair =
      subTable->find(txTreeNode->entryCallStack, found);
  if (!found) {
    if (debugLevel >= 1) {
      klee_message("#%lu: Check failure due to entry not found",
                   state.txTreeNode->getNodeSequenceNumber());
    }
    return false;
  }

  if (iterPair.first != iterPair.second) {

    std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
    storedExpressions =
        txTreeNode->getStoredExpressions(txTreeNode->entryCallStack);

    // Iterate the subsumption table entry with reverse iterator because
    // the successful subsumption mostly happen in the newest entry.
    for (EntryIterator it = iterPair.first, ie = iterPair.second; it != ie;
         ++it) {
      if ((*it)->subsumed(solver, state, timeout, storedExpressions,
                          debugLevel)) {
        // We mark as subsumed such that the node will not be
        // stored into table (the table already contains a more
        // general entry).
        txTreeNode->isSubsumed = true;

        // Mark the node as subsumed, and create a subsumption edge
        TxTreeGraph::markAsSubsumed(txTreeNode, (*it));
        return true;
      }
    }
  }
  return false;
}

void SubsumptionTable::clear() {
  for (std::map<uintptr_t, StackIndexedTable *>::iterator it = instance.begin(),
                                                          ie = instance.end();
       it != ie; ++it) {
    if (it->second) {
      ++TxTree::programPointNumber;
      delete it->second;
    }
  }
}

/**/

Statistic TxTree::setCurrentINodeTime("SetCurrentINodeTime",
                                      "SetCurrentINodeTime");
Statistic TxTree::removeTime("RemoveTime", "RemoveTime");
Statistic TxTree::subsumptionCheckTime("SubsumptionCheckTime",
                                       "SubsumptionCheckTime");
Statistic TxTree::markPathConditionTime("MarkPathConditionTime", "MarkPCTime");
Statistic TxTree::splitTime("SplitTime", "SplitTime");
Statistic TxTree::executeOnNodeTime("ExecuteOnNodeTime", "ExecuteOnNodeTime");
Statistic TxTree::executeMemoryOperationTime("ExecuteMemoryOperationTime",
                                             "ExecuteMemoryOperationTime");

double TxTree::entryNumber;

double TxTree::programPointNumber;

bool TxTree::symbolicExecutionError = false;

uint64_t TxTree::subsumptionCheckCount = 0;

void TxTree::printTimeStat(std::stringstream &stream) {
  stream << "KLEE: done:     setCurrentINode = "
         << ((double)setCurrentINodeTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     remove = " << ((double)removeTime.getValue()) /
                                               1000 << "\n";
  stream << "KLEE: done:     subsumptionCheck = "
         << ((double)subsumptionCheckTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     markPathCondition = "
         << ((double)markPathConditionTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     split = " << ((double)splitTime.getValue()) / 1000
         << "\n";
  stream << "KLEE: done:     executeOnNode = "
         << ((double)executeOnNodeTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     executeMemoryOperation = "
         << ((double)executeMemoryOperationTime.getValue()) / 1000 << "\n";
}

void TxTree::printTableStat(std::stringstream &stream) {
  SubsumptionTableEntry::printStat(stream);

  stream
      << "KLEE: done:     Average table entries per subsumption checkpoint = "
      << inTwoDecimalPoints(entryNumber / programPointNumber) << "\n";

  stream << "KLEE: done:     Number of subsumption checks = "
         << subsumptionCheckCount << "\n";

  stream << "KLEE: done:     Average solver calls per subsumption check = "
         << inTwoDecimalPoints((double)stats::subsumptionQueryCount /
                               (double)subsumptionCheckCount) << "\n";
}

std::string TxTree::inTwoDecimalPoints(const double n) {
  std::ostringstream stream;
  unsigned long x = (unsigned)((n - ((unsigned)n)) * 100);
  unsigned y = (unsigned)n;
  stream << y << ".";
  if (x > 9)
    stream << x;
  else if (x > 0)
    stream << "0" << x;
  else
    stream << "00";
  return stream.str();
}

std::string TxTree::getInterpolationStat() {
  std::stringstream stream;
  stream << "\nKLEE: done: Subsumption statistics\n";
  printTableStat(stream);
  stream << "KLEE: done: TxTree method execution times (ms):\n";
  printTimeStat(stream);
  stream << "KLEE: done: TxTreeNode method execution times (ms):\n";
  TxTreeNode::printTimeStat(stream);
  return stream.str();
}

TxTree::TxTree(ExecutionState *_root, llvm::DataLayout *_targetData)
    : targetData(_targetData) {
  currentINode = 0;
  assert(_targetData && "target data layout not provided");
  if (!_root->txTreeNode) {
    currentINode = new TxTreeNode(0, _targetData);
  }
  root = currentINode;
}

bool TxTree::subsumptionCheck(TimingSolver *solver, ExecutionState &state,
                              double timeout) {
#ifdef ENABLE_Z3
  assert(state.txTreeNode == currentINode);

  // Immediately return if the state's instruction is not the
  // the interpolation node id. The interpolation node id is the
  // first instruction executed of the sequence executed for a state
  // node, typically this the first instruction of a basic block.
  // Subsumption check only matches against this first instruction.
  if (!state.txTreeNode || reinterpret_cast<uintptr_t>(state.pc->inst) !=
                               state.txTreeNode->getProgramPoint())
    return false;

  int debugLevel = currentINode->dependency->debugLevel;

  if (debugLevel >= 2) {
    klee_message("Subsumption check for Node #%lu, Program Point %lu",
                 state.txTreeNode->getNodeSequenceNumber(),
                 state.txTreeNode->getProgramPoint());
  } else if (debugLevel >= 1) {
    klee_message("Subsumption check for Node #%lu",
                 state.txTreeNode->getNodeSequenceNumber());
  }

  ++subsumptionCheckCount; // For profiling

  TimerStatIncrementer t(subsumptionCheckTime);

  return SubsumptionTable::check(solver, state, timeout, debugLevel);
#endif
  return false;
}

void TxTree::setCurrentINode(ExecutionState &state) {
  TimerStatIncrementer t(setCurrentINodeTime);
  currentINode = state.txTreeNode;
  currentINode->setProgramPoint(state.pc->inst);
  TxTreeGraph::setCurrentNode(state, currentINode->nodeSequenceNumber);
}

void TxTree::remove(TxTreeNode *node) {
  int debugLevel = currentINode->dependency->debugLevel;

#ifdef ENABLE_Z3
  TimerStatIncrementer t(removeTime);
  assert(!node->left && !node->right);
  do {
    TxTreeNode *p = node->parent;

    // As the node is about to be deleted, it must have been completely
    // traversed, hence the correct time to table the interpolant.
    if (!node->isSubsumed && node->storable) {
      if (debugLevel >= 2) {
        klee_message("Storing entry for Node #%lu, Program Point %lu",
                     node->getNodeSequenceNumber(), node->getProgramPoint());
      } else if (debugLevel >= 1) {
        klee_message("Storing entry for Node #%lu",
                     node->getNodeSequenceNumber());
      }

      SubsumptionTableEntry *entry =
          new SubsumptionTableEntry(node, node->entryCallStack);
      SubsumptionTable::insert(node->getProgramPoint(), node->entryCallStack,
                               entry);

      TxTreeGraph::addTableEntryMapping(node, entry);

      if (debugLevel >= 2) {
        std::string msg;
        llvm::raw_string_ostream out(msg);
        entry->print(out);
        out.flush();
        klee_message("%s", msg.c_str());
      }
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
#endif
}

std::pair<TxTreeNode *, TxTreeNode *>
TxTree::split(TxTreeNode *parent, ExecutionState *left, ExecutionState *right) {
  TimerStatIncrementer t(splitTime);
  parent->split(left, right);
  TxTreeGraph::addChildren(parent, parent->left, parent->right);
  std::pair<TxTreeNode *, TxTreeNode *> ret(parent->left, parent->right);
  return ret;
}

void TxTree::markPathCondition(ExecutionState &state, TimingSolver *solver) {
  TimerStatIncrementer t(markPathConditionTime);
  const std::vector<ref<Expr> > &unsatCore = solver->getUnsatCore();

  int debugLevel = currentINode->dependency->debugLevel;

  llvm::BranchInst *binst =
      llvm::dyn_cast<llvm::BranchInst>(state.prevPC->inst);
  if (binst) {
    ref<Expr> unknownExpression;
    std::string reason = "";
    if (debugLevel >= 1) {
      llvm::raw_string_ostream stream(reason);
      stream << "branch infeasibility [";
      if (binst->getParent()->getParent()) {
        stream << binst->getParent()->getParent()->getName().str() << ": ";
      }
      if (llvm::MDNode *n = binst->getMetadata("dbg")) {
        llvm::DILocation loc(n);
        stream << "Line " << loc.getLineNumber();
      } else {
        binst->print(stream);
      }
      stream << "]";
      stream.flush();
    }
    currentINode->dependency->markAllValues(binst->getCondition(),
                                            unknownExpression, reason);
  }

  PathCondition *pc = currentINode->pathCondition;

  if (pc != 0) {
    for (std::vector<ref<Expr> >::const_iterator it = unsatCore.begin(),
                                                 ie = unsatCore.end();
         it != ie; ++it) {
      for (; pc != 0; pc = pc->cdr()) {
        if (pc->car().compare(it->get()) == 0) {
          pc->setAsCore(debugLevel);
          pc = pc->cdr();
          break;
        }
      }
    }
  }
}

void TxTree::execute(llvm::Instruction *instr) {
  std::vector<ref<Expr> > dummyArgs;
  executeOnNode(currentINode, instr, dummyArgs);
}

void TxTree::execute(llvm::Instruction *instr, ref<Expr> arg1) {
  std::vector<ref<Expr> > args;
  args.push_back(arg1);
  executeOnNode(currentINode, instr, args);
}

void TxTree::execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2) {
  std::vector<ref<Expr> > args;
  args.push_back(arg1);
  args.push_back(arg2);
  executeOnNode(currentINode, instr, args);
}

void TxTree::execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2,
                     ref<Expr> arg3) {
  std::vector<ref<Expr> > args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  executeOnNode(currentINode, instr, args);
}

void TxTree::execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args) {
  executeOnNode(currentINode, instr, args);
}

void TxTree::executePHI(llvm::Instruction *instr, unsigned incomingBlock,
                        ref<Expr> valueExpr) {
  currentINode->dependency->executePHI(instr, incomingBlock,
                                       currentINode->callStack, valueExpr,
                                       symbolicExecutionError);
  symbolicExecutionError = false;
}

void TxTree::executeOnNode(TxTreeNode *node, llvm::Instruction *instr,
                           std::vector<ref<Expr> > &args) {
  TimerStatIncrementer t(executeOnNodeTime);
  node->execute(instr, args, symbolicExecutionError);
  symbolicExecutionError = false;
}

void TxTree::printNode(llvm::raw_ostream &stream, TxTreeNode *n,
                       std::string edges) const {
  if (n->left != 0) {
    stream << "\n";
    stream << edges << "+-- L:" << n->left->programPoint;
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
    stream << edges << "+-- R:" << n->right->programPoint;
    if (this->currentINode == n->right) {
      stream << " (active)";
    }
    printNode(stream, n->right, edges + "    ");
  }
}

void TxTree::print(llvm::raw_ostream &stream) const {
  stream << "------------------------- TxTree Structure "
            "--------------------------\n";
  stream << this->root->programPoint;
  if (this->root == this->currentINode) {
    stream << " (active)";
  }
  this->printNode(stream, this->root, "");
  stream << "\n------------------------- Subsumption Table "
            "-------------------------\n";
  SubsumptionTable::print(stream);
}

void TxTree::dump() const { this->print(llvm::errs()); }

/**/

// Statistics
Statistic TxTreeNode::getInterpolantTime("GetInterpolantTime",
                                         "GetInterpolantTime");
Statistic TxTreeNode::addConstraintTime("AddConstraintTime",
                                        "AddConstraintTime");
Statistic TxTreeNode::splitTime("SplitTime", "SplitTime");
Statistic TxTreeNode::executeTime("ExecuteTime", "ExecuteTime");
Statistic TxTreeNode::bindCallArgumentsTime("BindCallArgumentsTime",
                                            "BindCallArgumentsTime");
Statistic TxTreeNode::bindReturnValueTime("BindReturnValueTime",
                                          "BindReturnValueTime");
Statistic TxTreeNode::getStoredExpressionsTime("GetStoredExpressionsTime",
                                               "GetStoredExpressionsTime");
Statistic
TxTreeNode::getStoredCoreExpressionsTime("GetStoredCoreExpressionsTime",
                                         "GetStoredCoreExpressionsTime");

// The interpolation tree node sequence number
uint64_t TxTreeNode::nextNodeSequenceNumber = 1;

void TxTreeNode::printTimeStat(std::stringstream &stream) {
  stream << "KLEE: done:     getInterpolant = "
         << ((double)getInterpolantTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     addConstraintTime = "
         << ((double)addConstraintTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     splitTime = " << ((double)splitTime.getValue()) /
                                                  1000 << "\n";
  stream << "KLEE: done:     execute = " << ((double)executeTime.getValue()) /
                                                1000 << "\n";
  stream << "KLEE: done:     bindCallArguments = "
         << ((double)bindCallArgumentsTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     bindReturnValue = "
         << ((double)bindReturnValueTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     getStoredExpressions = "
         << ((double)getStoredExpressionsTime.getValue()) / 1000 << "\n";
  stream << "KLEE: done:     getStoredCoreExpressions = "
         << ((double)getStoredCoreExpressionsTime.getValue()) / 1000 << "\n";
}

TxTreeNode::TxTreeNode(TxTreeNode *_parent, llvm::DataLayout *_targetData)
    : parent(_parent), left(0), right(0), programPoint(0),
      nodeSequenceNumber(nextNodeSequenceNumber++), storable(true),
      graph(_parent ? _parent->graph : 0),
      instructionsDepth(_parent ? _parent->instructionsDepth : 0),
      targetData(_targetData), isSubsumed(false) {

  pathCondition = 0;
  if (_parent) {
    pathCondition = _parent->pathCondition;
    entryCallStack = _parent->callStack;
    callStack = _parent->callStack;
  }

  // Inherit the abstract dependency or NULL
  dependency = new Dependency(_parent ? _parent->dependency : 0, _targetData);
}

TxTreeNode::~TxTreeNode() {
  // Only delete the path condition if it's not
  // also the parent's path condition
  PathCondition *ie = parent ? parent->pathCondition : 0;

  PathCondition *it = pathCondition;
  while (it != ie) {
    PathCondition *tmp = it;
    it = it->cdr();
    delete tmp;
  }

  if (dependency)
    delete dependency;
}

ref<Expr>
TxTreeNode::getInterpolant(std::set<const Array *> &replacements) const {
  TimerStatIncrementer t(getInterpolantTime);
  ref<Expr> expr = this->pathCondition->packInterpolant(replacements);
  return expr;
}

void TxTreeNode::addConstraint(ref<Expr> &constraint, llvm::Value *condition) {
  TimerStatIncrementer t(addConstraintTime);
  pathCondition = new PathCondition(constraint, dependency, condition,
                                    callStack, pathCondition);
  graph->addPathCondition(this, pathCondition, constraint);
}

void TxTreeNode::split(ExecutionState *leftData, ExecutionState *rightData) {
  TimerStatIncrementer t(splitTime);
  assert(left == 0 && right == 0);
  leftData->txTreeNode = left = new TxTreeNode(this, targetData);
  rightData->txTreeNode = right = new TxTreeNode(this, targetData);
}

void TxTreeNode::execute(llvm::Instruction *instr,
                         std::vector<ref<Expr> > &args,
                         bool symbolicExecutionError) {
  TimerStatIncrementer t(executeTime);
  dependency->execute(instr, callStack, args, symbolicExecutionError);
}

void TxTreeNode::bindCallArguments(llvm::Instruction *site,
                                   std::vector<ref<Expr> > &arguments) {
  TimerStatIncrementer t(bindCallArgumentsTime);
  dependency->bindCallArguments(site, callStack, arguments);
}

void TxTreeNode::bindReturnValue(llvm::CallInst *site, llvm::Instruction *inst,
                                 ref<Expr> returnValue) {
  // TODO: This is probably where we should simplify
  // the dependency graph by removing callee values.
  TimerStatIncrementer t(bindReturnValueTime);
  dependency->bindReturnValue(site, callStack, inst, returnValue);
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
TxTreeNode::getStoredExpressions(const std::vector<llvm::Instruction *> &stack)
    const {
  TimerStatIncrementer t(getStoredExpressionsTime);
  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore> ret;
  std::set<const Array *> dummyReplacements;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret = parent->dependency->getStoredExpressions(stack, dummyReplacements,
                                                   false);
  return ret;
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
TxTreeNode::getStoredCoreExpressions(
    const std::vector<llvm::Instruction *> &stack,
    std::set<const Array *> &replacements) const {
  TimerStatIncrementer t(getStoredCoreExpressionsTime);
  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore> ret;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret = parent->dependency->getStoredExpressions(stack, replacements, true);
  return ret;
}

uint64_t TxTreeNode::getInstructionsDepth() { return instructionsDepth; }

void TxTreeNode::incInstructionsDepth() { ++instructionsDepth; }

void
TxTreeNode::unsatCoreInterpolation(const std::vector<ref<Expr> > &unsatCore) {
  // State subsumed, we mark needed constraints on the path condition. We create
  // path condition marking structure to mark core constraints
  std::map<Expr *, PathCondition *> markerMap;
  for (PathCondition *it = pathCondition; it != 0; it = it->cdr()) {
    if (llvm::isa<OrExpr>(it->car())) {
      // FIXME: Break up disjunction into its components, because each disjunct
      // is solved separately. The or constraint was due to state merge.
      // Hence, the following is just a makeshift for when state merge is
      // properly implemented.
      markerMap[it->car()->getKid(0).get()] = it;
      markerMap[it->car()->getKid(1).get()] = it;
    }
    markerMap[it->car().get()] = it;
  }

  for (std::vector<ref<Expr> >::const_iterator it1 = unsatCore.begin(),
                                               ie1 = unsatCore.end();
       it1 != ie1; ++it1) {
    // FIXME: Sometimes some constraints are not in the PC. This is
    // because constraints are not properly added at state merge.
    PathCondition *cond = markerMap[it1->get()];
    if (cond)
      cond->setAsCore(dependency->debugLevel);
  }
}

void TxTreeNode::dump() const {
  llvm::errs() << "------------------------- TxTree Node "
                  "-------------------------------\n";
  this->print(llvm::errs());
  llvm::errs() << "\n";
}

void TxTreeNode::print(llvm::raw_ostream &stream) const {
  this->print(stream, 0);
}

void TxTreeNode::print(llvm::raw_ostream &stream,
                       const unsigned paddingAmount) const {
  std::string tabs = makeTabs(paddingAmount);
  std::string tabsNext = appendTab(tabs);

  stream << tabs << "TxTreeNode\n";
  stream << tabsNext << "node Id = " << programPoint << "\n";
  stream << tabsNext << "pathCondition = ";
  if (pathCondition == 0) {
    stream << "NULL";
  } else {
    pathCondition->print(stream);
  }
  stream << "\n";
  stream << tabsNext << "Left:\n";
  if (!left) {
    stream << tabsNext << "NULL\n";
  } else {
    left->print(stream, paddingAmount + 1);
    stream << "\n";
  }
  stream << tabsNext << "Right:\n";
  if (!right) {
    stream << tabsNext << "NULL\n";
  } else {
    right->print(stream, paddingAmount + 1);
    stream << "\n";
  }
  stream << tabsNext << "Stack:\n";
  for (std::vector<llvm::Instruction *>::const_reverse_iterator
           it = callStack.rbegin(),
           ie = callStack.rend();
       it != ie; ++it) {
    stream << tabsNext;
    (*it)->print(stream);
    stream << "\n";
  }
  if (dependency) {
    stream << tabsNext << "------- Abstract Dependencies ----------\n";
    dependency->print(stream, paddingAmount + 1);
  }
}
