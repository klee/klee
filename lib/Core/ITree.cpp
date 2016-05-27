//===-- ITree.cpp - Interpolation tree --------------------------*- C++ -*-===//
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

#include "ITree.h"
#include "Dependency.h"
#include "TimingSolver.h"

#include <klee/CommandLine.h>
#include <klee/Expr.h>
#include <klee/Solver.h>
#include <klee/util/ExprPPrinter.h>
#include <fstream>
#include <vector>

using namespace klee;

/**/

std::string SearchTree::PrettyExpressionBuilder::bvConst32(uint32_t value) {
  std::ostringstream stream;
  stream << value;
  return stream.str();
}
std::string SearchTree::PrettyExpressionBuilder::bvConst64(uint64_t value) {
  std::ostringstream stream;
  stream << value;
  return stream.str();
}
std::string SearchTree::PrettyExpressionBuilder::bvZExtConst(uint64_t value) {
  return bvConst64(value);
}
std::string SearchTree::PrettyExpressionBuilder::bvSExtConst(uint64_t value) {
  return bvConst64(value);
}
std::string SearchTree::PrettyExpressionBuilder::bvBoolExtract(std::string expr,
                                                               int bit) {
  std::ostringstream stream;
  stream << expr << "[" << bit << "]";
  return stream.str();
}
std::string SearchTree::PrettyExpressionBuilder::bvExtract(std::string expr,
                                                           unsigned top,
                                                           unsigned bottom) {
  std::ostringstream stream;
  stream << expr << "[" << top << "," << bottom << "]";
  return stream.str();
}
std::string SearchTree::PrettyExpressionBuilder::eqExpr(std::string a,
                                                        std::string b) {
  if (a == "false")
    return "!" + b;
  return "(" + a + " = " + b + ")";
}

// logical left and right shift (not arithmetic)
std::string SearchTree::PrettyExpressionBuilder::bvLeftShift(std::string expr,
                                                             unsigned shift) {
  std::ostringstream stream;
  stream << "(" << expr << " \\<\\< " << shift << ")";
  return stream.str();
}
std::string SearchTree::PrettyExpressionBuilder::bvRightShift(std::string expr,
                                                              unsigned shift) {
  std::ostringstream stream;
  stream << "(" << expr << " \\>\\> " << shift << ")";
  return stream.str();
}
std::string
SearchTree::PrettyExpressionBuilder::bvVarLeftShift(std::string expr,
                                                    std::string shift) {
  return "(" + expr + " \\<\\< " + shift + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::bvVarRightShift(std::string expr,
                                                     std::string shift) {
  return "(" + expr + " \\>\\> " + shift + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::bvVarArithRightShift(std::string expr,
                                                          std::string shift) {
  return bvVarRightShift(expr, shift);
}

// Some STP-style bitvector arithmetic
std::string
SearchTree::PrettyExpressionBuilder::bvMinusExpr(std::string minuend,
                                                 std::string subtrahend) {
  return "(" + minuend + " - " + subtrahend + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::bvPlusExpr(std::string augend,
                                                std::string addend) {
  return "(" + augend + " + " + addend + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::bvMultExpr(std::string multiplacand,
                                                std::string multiplier) {
  return "(" + multiplacand + " * " + multiplier + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::bvDivExpr(std::string dividend,
                                               std::string divisor) {
  return "(" + dividend + " / " + divisor + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::sbvDivExpr(std::string dividend,
                                                std::string divisor) {
  return "(" + dividend + " / " + divisor + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::bvModExpr(std::string dividend,
                                               std::string divisor) {
  return "(" + dividend + " % " + divisor + ")";
}
std::string
SearchTree::PrettyExpressionBuilder::sbvModExpr(std::string dividend,
                                                std::string divisor) {
  return "(" + dividend + " % " + divisor + ")";
}
std::string SearchTree::PrettyExpressionBuilder::notExpr(std::string expr) {
  return "!(" + expr + ")";
}
std::string SearchTree::PrettyExpressionBuilder::bvAndExpr(std::string lhs,
                                                           std::string rhs) {
  return "(" + lhs + " & " + rhs + ")";
}
std::string SearchTree::PrettyExpressionBuilder::bvOrExpr(std::string lhs,
                                                          std::string rhs) {
  return "(" + lhs + " | " + rhs + ")";
}
std::string SearchTree::PrettyExpressionBuilder::iffExpr(std::string lhs,
                                                         std::string rhs) {
  return "(" + lhs + " \\<=\\> " + rhs + ")";
}
std::string SearchTree::PrettyExpressionBuilder::bvXorExpr(std::string lhs,
                                                           std::string rhs) {
  return "(" + lhs + " xor " + rhs + ")";
}
std::string SearchTree::PrettyExpressionBuilder::bvSignExtend(std::string src) {
  return src;
}

// Some STP-style array domain interface
std::string SearchTree::PrettyExpressionBuilder::writeExpr(std::string array,
                                                           std::string index,
                                                           std::string value) {
  return "update(" + array + "," + index + "," + value + ")";
}
std::string SearchTree::PrettyExpressionBuilder::readExpr(std::string array,
                                                          std::string index) {
  return array + "[" + index + "]";
}

// ITE-expression constructor
std::string SearchTree::PrettyExpressionBuilder::iteExpr(
    std::string condition, std::string whenTrue, std::string whenFalse) {
  return "ite(" + condition + "," + whenTrue + "," + whenFalse + ")";
}

// Bitvector comparison
std::string SearchTree::PrettyExpressionBuilder::bvLtExpr(std::string lhs,
                                                          std::string rhs) {
  return "(" + lhs + " \\< " + rhs + ")";
}
std::string SearchTree::PrettyExpressionBuilder::bvLeExpr(std::string lhs,
                                                          std::string rhs) {
  return "(" + lhs + " \\<= " + rhs + ")";
}
std::string SearchTree::PrettyExpressionBuilder::sbvLtExpr(std::string lhs,
                                                           std::string rhs) {
  return "(" + lhs + " \\< " + rhs + ")";
}
std::string SearchTree::PrettyExpressionBuilder::sbvLeExpr(std::string lhs,
                                                           std::string rhs) {
  return "(" + lhs + " \\<= " + rhs + ")";
}

std::string SearchTree::PrettyExpressionBuilder::constructAShrByConstant(
    std::string expr, unsigned shift, std::string isSigned) {
  return bvRightShift(expr, shift);
}
std::string
SearchTree::PrettyExpressionBuilder::constructMulByConstant(std::string expr,
                                                            uint64_t x) {
  std::ostringstream stream;
  stream << "(" << expr << " * " << x << ")";
  return stream.str();
}
std::string
SearchTree::PrettyExpressionBuilder::constructUDivByConstant(std::string expr_n,
                                                             uint64_t d) {
  std::ostringstream stream;
  stream << "(" << expr_n << " / " << d << ")";
  return stream.str();
}
std::string
SearchTree::PrettyExpressionBuilder::constructSDivByConstant(std::string expr_n,
                                                             uint64_t d) {
  std::ostringstream stream;
  stream << "(" << expr_n << " / " << d << ")";
  return stream.str();
}

std::string
SearchTree::PrettyExpressionBuilder::getInitialArray(const Array *root) {
  std::string array_expr =
      buildArray(root->name.c_str(), root->getDomain(), root->getRange());

  if (root->isConstantArray()) {
    for (unsigned i = 0, e = root->size; i != e; ++i) {
      std::string prev = array_expr;
      array_expr = writeExpr(
          prev, constructActual(ConstantExpr::alloc(i, root->getDomain())),
          constructActual(root->constantValues[i]));
    }
  }
  return array_expr;
}
std::string
SearchTree::PrettyExpressionBuilder::getArrayForUpdate(const Array *root,
                                                       const UpdateNode *un) {
  if (!un) {
    return (getInitialArray(root));
  }
  return writeExpr(getArrayForUpdate(root, un->next),
                   constructActual(un->index), constructActual(un->value));
}

std::string SearchTree::PrettyExpressionBuilder::constructActual(ref<Expr> e) {
  switch (e->getKind()) {
  case Expr::Constant: {
    ConstantExpr *CE = cast<ConstantExpr>(e);
    int width = CE->getWidth();

    // Coerce to bool if necessary.
    if (width == 1)
      return CE->isTrue() ? getTrue() : getFalse();

    // Fast path.
    if (width <= 32)
      return bvConst32(CE->getZExtValue(32));
    if (width <= 64)
      return bvConst64(CE->getZExtValue());

    ref<ConstantExpr> Tmp = CE;
    return bvConst64(Tmp->Extract(0, 64)->getZExtValue());
  }

  // Special
  case Expr::NotOptimized: {
    NotOptimizedExpr *noe = cast<NotOptimizedExpr>(e);
    return constructActual(noe->src);
  }

  case Expr::Read: {
    ReadExpr *re = cast<ReadExpr>(e);
    assert(re && re->updates.root);
    return readExpr(getArrayForUpdate(re->updates.root, re->updates.head),
                    constructActual(re->index));
  }

  case Expr::Select: {
    SelectExpr *se = cast<SelectExpr>(e);
    std::string cond = constructActual(se->cond);
    std::string tExpr = constructActual(se->trueExpr);
    std::string fExpr = constructActual(se->falseExpr);
    return iteExpr(cond, tExpr, fExpr);
  }

  case Expr::Concat: {
    ConcatExpr *ce = cast<ConcatExpr>(e);
    unsigned numKids = ce->getNumKids();
    std::string res = constructActual(ce->getKid(numKids - 1));
    for (int i = numKids - 2; i >= 0; i--) {
      res = constructActual(ce->getKid(i)) + "." + res;
    }
    return res;
  }

  case Expr::Extract: {
    ExtractExpr *ee = cast<ExtractExpr>(e);
    std::string src = constructActual(ee->expr);
    int width = ee->getWidth();
    if (width == 1) {
      return bvBoolExtract(src, ee->offset);
    } else {
      return bvExtract(src, ee->offset + width - 1, ee->offset);
    }
  }

  // Casting
  case Expr::ZExt: {
    CastExpr *ce = cast<CastExpr>(e);
    std::string src = constructActual(ce->src);
    int width = ce->getWidth();
    if (width == 1) {
      return iteExpr(src, bvOne(), bvZero());
    } else {
      return src;
    }
  }

  case Expr::SExt: {
    CastExpr *ce = cast<CastExpr>(e);
    std::string src = constructActual(ce->src);
    return bvSignExtend(src);
  }

  // Arithmetic
  case Expr::Add: {
    AddExpr *ae = cast<AddExpr>(e);
    std::string left = constructActual(ae->left);
    std::string right = constructActual(ae->right);
    return bvPlusExpr(left, right);
  }

  case Expr::Sub: {
    SubExpr *se = cast<SubExpr>(e);
    std::string left = constructActual(se->left);
    std::string right = constructActual(se->right);
    return bvMinusExpr(left, right);
  }

  case Expr::Mul: {
    MulExpr *me = cast<MulExpr>(e);
    std::string right = constructActual(me->right);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(me->left))
      if (CE->getWidth() <= 64)
        return constructMulByConstant(right, CE->getZExtValue());

    std::string left = constructActual(me->left);
    return bvMultExpr(left, right);
  }

  case Expr::UDiv: {
    UDivExpr *de = cast<UDivExpr>(e);
    std::string left = constructActual(de->left);

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();

        if (bits64::isPowerOfTwo(divisor)) {
          return bvRightShift(left, bits64::indexOfSingleBit(divisor));
        }
      }
    }

    std::string right = constructActual(de->right);
    return bvDivExpr(left, right);
  }

  case Expr::SDiv: {
    SDivExpr *de = cast<SDivExpr>(e);
    std::string left = constructActual(de->left);
    std::string right = constructActual(de->right);
    return sbvDivExpr(left, right);
  }

  case Expr::URem: {
    URemExpr *de = cast<URemExpr>(e);
    std::string left = constructActual(de->left);

    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(de->right)) {
      if (CE->getWidth() <= 64) {
        uint64_t divisor = CE->getZExtValue();

        if (bits64::isPowerOfTwo(divisor)) {
          unsigned bits = bits64::indexOfSingleBit(divisor);

          // special case for modding by 1 or else we bvExtract -1:0
          if (bits == 0) {
            return bvZero();
          } else {
            return bvExtract(left, bits - 1, 0);
          }
        }
      }
    }

    std::string right = constructActual(de->right);
    return bvModExpr(left, right);
  }

  case Expr::SRem: {
    SRemExpr *de = cast<SRemExpr>(e);
    std::string left = constructActual(de->left);
    std::string right = constructActual(de->right);
    return sbvModExpr(left, right);
  }

  // Bitwise
  case Expr::Not: {
    NotExpr *ne = cast<NotExpr>(e);
    std::string expr = constructActual(ne->expr);
    return notExpr(expr);
  }

  case Expr::And: {
    AndExpr *ae = cast<AndExpr>(e);
    std::string left = constructActual(ae->left);
    std::string right = constructActual(ae->right);
    return bvAndExpr(left, right);
  }

  case Expr::Or: {
    OrExpr *oe = cast<OrExpr>(e);
    std::string left = constructActual(oe->left);
    std::string right = constructActual(oe->right);
    return bvOrExpr(left, right);
  }

  case Expr::Xor: {
    XorExpr *xe = cast<XorExpr>(e);
    std::string left = constructActual(xe->left);
    std::string right = constructActual(xe->right);
    return bvXorExpr(left, right);
  }

  case Expr::Shl: {
    ShlExpr *se = cast<ShlExpr>(e);
    std::string left = constructActual(se->left);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(se->right)) {
      return bvLeftShift(left, (unsigned)CE->getLimitedValue());
    } else {
      std::string amount = constructActual(se->right);
      return bvVarLeftShift(left, amount);
    }
  }

  case Expr::LShr: {
    LShrExpr *lse = cast<LShrExpr>(e);
    std::string left = constructActual(lse->left);
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(lse->right)) {
      return bvRightShift(left, (unsigned)CE->getLimitedValue());
    } else {
      std::string amount = constructActual(lse->right);
      return bvVarRightShift(left, amount);
    }
  }

  case Expr::AShr: {
    AShrExpr *ase = cast<AShrExpr>(e);
    std::string left = constructActual(ase->left);
    std::string amount = constructActual(ase->right);
    return bvVarArithRightShift(left, amount);
  }

  // Comparison
  case Expr::Eq: {
    EqExpr *ee = cast<EqExpr>(e);
    std::string left = constructActual(ee->left);
    std::string right = constructActual(ee->right);
    return eqExpr(left, right);
  }

  case Expr::Ult: {
    UltExpr *ue = cast<UltExpr>(e);
    std::string left = constructActual(ue->left);
    std::string right = constructActual(ue->right);
    return bvLtExpr(left, right);
  }

  case Expr::Ule: {
    UleExpr *ue = cast<UleExpr>(e);
    std::string left = constructActual(ue->left);
    std::string right = constructActual(ue->right);
    return bvLeExpr(left, right);
  }

  case Expr::Slt: {
    SltExpr *se = cast<SltExpr>(e);
    std::string left = constructActual(se->left);
    std::string right = constructActual(se->right);
    return sbvLtExpr(left, right);
  }

  case Expr::Sle: {
    SleExpr *se = cast<SleExpr>(e);
    std::string left = constructActual(se->left);
    std::string right = constructActual(se->right);
    return sbvLeExpr(left, right);
  }

  case Expr::Exists: {
    ExistsExpr *xe = cast<ExistsExpr>(e);
    std::string existentials;

    for (std::set<const Array *>::iterator it = xe->variables.begin(),
                                           itEnd = xe->variables.end();
         it != itEnd; ++it) {
      existentials += (*it)->name;
      if (it != itEnd)
        existentials += ",";
    }

    return "(exists (" + existentials + ") " + constructActual(xe->body) + ")";
  }

  default:
    assert(0 && "unhandled Expr type");
    return getTrue();
  }
}
std::string SearchTree::PrettyExpressionBuilder::construct(ref<Expr> e) {
  PrettyExpressionBuilder *instance = new PrettyExpressionBuilder();
  std::string ret = instance->constructActual(e);
  delete instance;
  return ret;
}

std::string SearchTree::PrettyExpressionBuilder::buildArray(
    const char *name, unsigned indexWidth, unsigned valueWidth) {
  return name;
}

std::string SearchTree::PrettyExpressionBuilder::getTrue() { return "true"; }
std::string SearchTree::PrettyExpressionBuilder::getFalse() { return "false"; }
std::string
SearchTree::PrettyExpressionBuilder::getInitialRead(const Array *root,
                                                    unsigned index) {
  return readExpr(getInitialArray(root), bvConst32(index));
}

SearchTree::PrettyExpressionBuilder::PrettyExpressionBuilder() {}

SearchTree::PrettyExpressionBuilder::~PrettyExpressionBuilder() {}

/**/

std::string SearchTree::NumberedEdge::render() const {
  std::ostringstream stream;
  stream << "Node" << source->nodeId << " -> Node" << destination->nodeId
         << " [style=dashed,label=\"" << number << "\"];";
  return stream.str();
}

/**/

unsigned long SearchTree::nextNodeId = 1;

SearchTree *SearchTree::instance = 0;

std::string SearchTree::recurseRender(const SearchTree::Node *node) {
  std::ostringstream stream;

  stream << "Node" << node->nodeId;
  std::string sourceNodeName = stream.str();

  stream << " [shape=record,label=\"{" << node->nodeId << ": " << node->name
         << "\\l";
  for (std::map<PathCondition *, std::pair<std::string, bool> >::const_iterator
           it = node->pathConditionTable.begin(),
           itEnd = node->pathConditionTable.end();
       it != itEnd; ++it) {
    stream << (it->second.first);
    if (it->second.second)
      stream << " ITP";
    stream << "\\l";
  }
  if (node->subsumed) {
    stream << "(subsumed)\\l";
  }
  if (node->falseTarget || node->trueTarget)
    stream << "|{<s0>F|<s1>T}";
  stream << "}\"];\n";

  if (node->falseTarget) {
    stream << sourceNodeName << ":s0 -> Node" << node->falseTarget->nodeId
           << ";\n";
  }
  if (node->trueTarget) {
    stream << sourceNodeName + ":s1 -> Node" << node->trueTarget->nodeId
           << ";\n";
  }
  if (node->falseTarget) {
    stream << recurseRender(node->falseTarget);
  }
  if (node->trueTarget) {
    stream << recurseRender(node->trueTarget);
  }
  return stream.str();
}

std::string SearchTree::render() {
  std::string res("");

  // Simply return empty string when root is undefined
  if (!root)
    return res;

  std::ostringstream stream;
  for (std::vector<SearchTree::NumberedEdge *>::iterator
           it = subsumptionEdges.begin(),
           itEnd = subsumptionEdges.end();
       it != itEnd; ++it) {
    stream << (*it)->render() << "\n";
  }

  res = "digraph search_tree {\n";
  res += recurseRender(root);
  res += stream.str();
  res += "}\n";
  return res;
}

SearchTree::SearchTree(ITreeNode *_root) : subsumptionEdgeNumber(0) {
  root = SearchTree::Node::createNode(_root->getNodeId());
  itreeNodeMap[_root] = root;
}

SearchTree::~SearchTree() {
  if (root)
    delete root;

  itreeNodeMap.clear();

  for (std::vector<SearchTree::NumberedEdge *>::iterator
           it = subsumptionEdges.begin(),
           itEnd = subsumptionEdges.end();
       it != itEnd; ++it) {
    delete *it;
  }
  subsumptionEdges.clear();
}

void SearchTree::addChildren(ITreeNode *parent, ITreeNode *falseChild,
                             ITreeNode *trueChild) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(SearchTree::instance && "Search tree graph not initialized");

  SearchTree::Node *parentNode = instance->itreeNodeMap[parent];
  parentNode->falseTarget =
      SearchTree::Node::createNode(falseChild->getNodeId());
  parentNode->trueTarget = SearchTree::Node::createNode(trueChild->getNodeId());
  instance->itreeNodeMap[falseChild] = parentNode->falseTarget;
  instance->itreeNodeMap[trueChild] = parentNode->trueTarget;
}

void SearchTree::setCurrentNode(ExecutionState &state,
                                const uintptr_t programPoint) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(SearchTree::instance && "Search tree graph not initialized");

  ITreeNode *iTreeNode = state.itreeNode;
  SearchTree::Node *node = instance->itreeNodeMap[iTreeNode];
  if (!node->nodeId) {
    std::string functionName(
        state.pc->inst->getParent()->getParent()->getName().str());
    node->name = functionName + "\\l";
    llvm::raw_string_ostream out(node->name);
    state.pc->inst->print(out);
    node->name = out.str();

    node->iTreeNodeId = programPoint;
    node->nodeId = nextNodeId++;
  }
}

void SearchTree::markAsSubsumed(ITreeNode *iTreeNode,
                                SubsumptionTableEntry *entry) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(SearchTree::instance && "Search tree graph not initialized");

  SearchTree::Node *node = instance->itreeNodeMap[iTreeNode];
  node->subsumed = true;
  SearchTree::Node *subsuming = instance->tableEntryMap[entry];
  instance->subsumptionEdges.push_back(new SearchTree::NumberedEdge(
      node, subsuming, ++(instance->subsumptionEdgeNumber)));
}

void SearchTree::addPathCondition(ITreeNode *iTreeNode,
                                  PathCondition *pathCondition,
                                  ref<Expr> condition) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(SearchTree::instance && "Search tree graph not initialized");

  SearchTree::Node *node = instance->itreeNodeMap[iTreeNode];

  std::string s = PrettyExpressionBuilder::construct(condition);

  std::pair<std::string, bool> p(s, false);
  node->pathConditionTable[pathCondition] = p;
  instance->pathConditionMap[pathCondition] = node;
}

void SearchTree::addTableEntryMapping(ITreeNode *iTreeNode,
                                      SubsumptionTableEntry *entry) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(SearchTree::instance && "Search tree graph not initialized");

  SearchTree::Node *node = instance->itreeNodeMap[iTreeNode];
  instance->tableEntryMap[entry] = node;
}

void SearchTree::setAsCore(PathCondition *pathCondition) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(SearchTree::instance && "Search tree graph not initialized");

  instance->pathConditionMap[pathCondition]->pathConditionTable[pathCondition].second = true;
}

/// @brief Save the graph
void SearchTree::save(std::string dotFileName) {
  if (!OUTPUT_INTERPOLATION_TREE)
    return;

  assert(SearchTree::instance && "Search tree graph not initialized");

  std::string g(instance->render());
  std::ofstream out(dotFileName.c_str());
  if (!out.fail()) {
    out << g;
    out.close();
  }
}

/**/

PathCondition::PathCondition(ref<Expr> &constraint, Dependency *dependency,
                             llvm::Value *condition, PathCondition *prev)
    : constraint(constraint), shadowConstraint(constraint), shadowed(false),
      dependency(dependency),
      condition(dependency ? dependency->getLatestValue(condition, constraint)
                           : 0),
      core(false), tail(prev) {}

PathCondition::~PathCondition() {}

ref<Expr> PathCondition::car() const { return constraint; }

PathCondition *PathCondition::cdr() const { return tail; }

void PathCondition::setAsCore(AllocationGraph *g) {
  // We mark all values to which this constraint depends
  dependency->markAllValues(g, condition);

  // We mark this constraint itself as core
  core = true;

  // We mark constraint as core in the search tree graph as well.
  SearchTree::setAsCore(this);
}

bool PathCondition::isCore() const { return core; }

ref<Expr>
PathCondition::packInterpolant(std::set<const Array *> &replacements) {
  ref<Expr> res;
  for (PathCondition *it = this; it != 0; it = it->tail) {
    if (it->core) {
      if (!it->shadowed) {
        it->shadowConstraint =
            (NoExistential ? it->constraint
                           : ShadowArray::getShadowExpression(it->constraint,
                                                              replacements));
        it->shadowed = true;
        it->boundVariables.insert(replacements.begin(), replacements.end());
      } else {
        // Already shadowed, we add the bound variables
        replacements.insert(it->boundVariables.begin(),
                            it->boundVariables.end());
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

void PathCondition::print(llvm::raw_ostream &stream) {
  stream << "[";
  for (PathCondition *it = this; it != 0; it = it->tail) {
    it->constraint->print(stream);
    stream << ": " << (it->core ? "core" : "non-core");
    if (it->tail != 0)
      stream << ",";
  }
  stream << "]";
}

/**/

StatTimer SubsumptionTableEntry::actualSolverCallTimer;

unsigned long SubsumptionTableEntry::checkSolverCount = 0;

unsigned long SubsumptionTableEntry::checkSolverFailureCount = 0;

SubsumptionTableEntry::SubsumptionTableEntry(ITreeNode *node)
    : nodeId(node->getNodeId()) {
  std::set<const Array *> replacements;

  interpolant = node->getInterpolant(replacements);

  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  storedExpressions = node->getStoredCoreExpressions(replacements);

  concreteAddressStore = storedExpressions.first;
  for (Dependency::ConcreteStore::iterator it = concreteAddressStore.begin(),
                                           itEnd = concreteAddressStore.end();
       it != itEnd; ++it) {
    concreteAddressStoreKeys.push_back(it->first);
  }

  symbolicAddressStore = storedExpressions.second;
  for (Dependency::SymbolicStore::iterator it = symbolicAddressStore.begin(),
                                           itEnd = symbolicAddressStore.end();
       it != itEnd; ++it) {
    symbolicAddressStoreKeys.push_back(it->first);
  }

  existentials = replacements;
}

SubsumptionTableEntry::~SubsumptionTableEntry() {}

bool
SubsumptionTableEntry::hasExistentials(std::set<const Array *> &existentials,
                                       ref<Expr> expr) {
  for (int i = 0, numKids = expr->getNumKids(); i < numKids; ++i) {
    if (llvm::isa<ReadExpr>(expr)) {
      ReadExpr *readExpr = llvm::dyn_cast<ReadExpr>(expr.get());
      const Array *array = (readExpr->updates).root;
      for (std::set<const Array *>::iterator it = existentials.begin(),
                                             itEnd = existentials.end();
           it != itEnd; ++it) {
        if ((*it) == array)
          return true;
      }
    } else if (hasExistentials(existentials, expr->getKid(i)))
      return true;
  }
  return false;
}

bool SubsumptionTableEntry::hasFree(std::set<const Array *> &existentials,
                                    ref<Expr> expr) {
  for (int i = 0, numKids = expr->getNumKids(); i < numKids; ++i) {
    if (llvm::isa<ReadExpr>(expr)) {
      ReadExpr *readExpr = llvm::dyn_cast<ReadExpr>(expr.get());
      const Array *array = (readExpr->updates).root;
      for (std::set<const Array *>::iterator it = existentials.begin(),
                                             itEnd = existentials.end();
           it != itEnd; ++it) {
        if ((*it) == array)
          return false;
      }
      return true;
    } else if (hasFree(existentials, expr->getKid(i)))
      return true;
  }
  return false;
}

ref<Expr>
SubsumptionTableEntry::simplifyWithFourierMotzkin(ref<Expr> existsExpr) {
  // This is a template for Fourier-Motzkin elimination. For now,
  // we simply return the input argument.
  return existsExpr;
}

ref<Expr>
SubsumptionTableEntry::simplifyArithmeticBody(ref<Expr> existsExpr,
                                              bool &hasExistentialsOnly) {
  assert(llvm::isa<ExistsExpr>(existsExpr.get()));

  ExistsExpr *expr = static_cast<ExistsExpr *>(existsExpr.get());

  // Assume the we shall return general ExistsExpr that does not contain
  // only existential variables.
  hasExistentialsOnly = false;

  std::set<const Array *> boundVariables = expr->variables;
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
    hasExistentialsOnly = !hasFree(expr->variables, simplifiedInterpolant);
    if (hasExistentialsOnly) {
      return existsExpr->rebuild(&simplifiedInterpolant);
    }
  }

  ref<Expr> newInterpolant;

  for (std::vector<ref<Expr> >::iterator it = interpolantPack.begin(),
                                         itEnd = interpolantPack.end();
       it != itEnd; ++it) {

    ref<Expr> interpolantAtom = (*it); // For example C cmp D

    // only process the interpolant that still has existential variables in it.
    if (hasExistentials(boundVariables, interpolantAtom)) {
      for (std::vector<ref<Expr> >::iterator itEq = equalityPack.begin(),
                                             itEqEnd = equalityPack.end();
           itEq != itEqEnd; ++itEq) {

        ref<Expr> equalityConstraint =
            *itEq; // For example, say this constraint is A == B
        if (equalityConstraint->isFalse()) {
          return ConstantExpr::alloc(0, Expr::Bool);
        } else if (equalityConstraint->isTrue()) {
          return ConstantExpr::alloc(1, Expr::Bool);
        }
        // Left-hand side of the equality formula (A in our example) that
        // contains
        // the shadow expression (we assume that the existentially-quantified
        // shadow variable is always on the left side).
        ref<Expr> equalityConstraintLeft = equalityConstraint->getKid(0);

        // Right-hand side of the equality formula (B in our example) that does
        // not contain existentially-quantified shadow variables.
        ref<Expr> equalityConstraintRight = equalityConstraint->getKid(1);

        ref<Expr> newIntpLeft;
        ref<Expr> newIntpRight;

        // When the if condition holds, we perform substitution
        if (containShadowExpr(equalityConstraintLeft,
                              interpolantAtom->getKid(0))) {
          // Here we perform substitution, where given
          // an interpolant atom and an equality constraint,
          // we try to find a subexpression in the equality constraint
          // that matches the lhs expression of the interpolant atom.

          // Here we assume that the equality constraint is A == B and the
          // interpolant atom is C cmp D.

          // newIntpLeft == B
          newIntpLeft = equalityConstraintRight;

          // If equalityConstraintLeft does not have any arithmetic operation
          // we could directly assign newIntpRight = D, otherwise,
          // newIntpRight == A[D/C]
          if (!llvm::isa<BinaryExpr>(equalityConstraintLeft))
            newIntpRight = interpolantAtom->getKid(1);
          else {
            // newIntpRight is A, but with every occurrence of C replaced with D
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
    if (newInterpolant.get()) {
      newInterpolant = AndExpr::alloc(newInterpolant, interpolantAtom);
    } else {
      newInterpolant = interpolantAtom;
    }
  }

  ref<Expr> newBody;

  if (newInterpolant.get()) {
    if (!hasExistentials(expr->variables, newInterpolant))
      return newInterpolant;

    newBody = AndExpr::alloc(newInterpolant, fullEqualityConstraint);
  } else {
    newBody = AndExpr::alloc(simplifiedInterpolant, fullEqualityConstraint);
  }

  return simplifyWithFourierMotzkin(existsExpr->rebuild(&newBody));
}

ref<Expr> SubsumptionTableEntry::replaceExpr(ref<Expr> originalExpr,
                                             ref<Expr> replacedExpr,
                                             ref<Expr> substituteExpr) {
  // We only handle binary expressions
  if (!llvm::isa<BinaryExpr>(originalExpr) ||
      llvm::isa<ConcatExpr>(originalExpr))
    return originalExpr;

  if (originalExpr->getKid(0) == replacedExpr)
    return ShadowArray::createBinaryOfSameKind(originalExpr, substituteExpr,
                                               originalExpr->getKid(1));

  if (originalExpr->getKid(1) == replacedExpr)
    return ShadowArray::createBinaryOfSameKind(
        originalExpr, originalExpr->getKid(0), substituteExpr);

  return ShadowArray::createBinaryOfSameKind(
      originalExpr,
      replaceExpr(originalExpr->getKid(0), replacedExpr, substituteExpr),
      replaceExpr(originalExpr->getKid(1), replacedExpr, substituteExpr));
}

bool SubsumptionTableEntry::containShadowExpr(ref<Expr> expr,
                                              ref<Expr> shadowExpr) {
  if (expr.operator==(shadowExpr))
    return true;
  if (expr->getNumKids() < 2 && expr.operator!=(shadowExpr))
    return false;

  return containShadowExpr(expr->getKid(0), shadowExpr) ||
         containShadowExpr(expr->getKid(1), shadowExpr);
}

ref<Expr> SubsumptionTableEntry::simplifyInterpolantExpr(
    std::vector<ref<Expr> > &interpolantPack, ref<Expr> expr) {
  if (expr->getNumKids() < 2)
    return expr;

  if (llvm::isa<EqExpr>(expr) && llvm::isa<ConstantExpr>(expr->getKid(0)) &&
      llvm::isa<ConstantExpr>(expr->getKid(1))) {
    return (expr->getKid(0).operator==(expr->getKid(1)))
               ? ConstantExpr::alloc(1, Expr::Bool)
               : ConstantExpr::alloc(0, Expr::Bool);
  } else if (llvm::isa<NeExpr>(expr) &&
             llvm::isa<ConstantExpr>(expr->getKid(0)) &&
             llvm::isa<ConstantExpr>(expr->getKid(1))) {
    return (expr->getKid(0).operator!=(expr->getKid(1)))
               ? ConstantExpr::alloc(1, Expr::Bool)
               : ConstantExpr::alloc(0, Expr::Bool);
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

  return AndExpr::alloc(simplifiedLhs, simplifiedRhs);
}

ref<Expr> SubsumptionTableEntry::simplifyEqualityExpr(
    std::vector<ref<Expr> > &equalityPack, ref<Expr> expr) {
  if (expr->getNumKids() < 2)
    return expr;

  if (llvm::isa<EqExpr>(expr)) {
    if (llvm::isa<ConstantExpr>(expr->getKid(0)) &&
        llvm::isa<ConstantExpr>(expr->getKid(1))) {
      return (expr->getKid(0).operator==(expr->getKid(1)))
                 ? ConstantExpr::alloc(1, Expr::Bool)
                 : ConstantExpr::alloc(0, Expr::Bool);
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

    return AndExpr::alloc(lhs, rhs);
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

    return OrExpr::alloc(lhs, rhs);
  }

  assert(!"Invalid expression type.");
}

ref<Expr>
SubsumptionTableEntry::getSubstitution(ref<Expr> equalities,
                                       std::map<ref<Expr>, ref<Expr> > &map) {
  // It is assumed the lhs is an expression on the existentially-quantified
  // variable whereas
  // the rhs is an expression on the free variables.
  if (llvm::isa<EqExpr>(equalities.get())) {
    ref<Expr> lhs = equalities->getKid(0);
    if (llvm::isa<ReadExpr>(lhs.get()) || llvm::isa<ConcatExpr>(lhs.get())) {
      map[lhs] = equalities->getKid(1);
      return ConstantExpr::alloc(1, Expr::Bool);
    }
    return equalities;
  }

  if (llvm::isa<AndExpr>(equalities.get())) {
    ref<Expr> lhs = getSubstitution(equalities->getKid(0), map);
    ref<Expr> rhs = getSubstitution(equalities->getKid(1), map);
    if (lhs->isTrue()) {
      if (rhs->isTrue()) {
        return ConstantExpr::alloc(1, Expr::Bool);
      }
      return rhs;
    } else {
      if (rhs->isTrue()) {
        return lhs;
      }
      return AndExpr::alloc(lhs, rhs);
    }
  }
  return equalities;
}

ref<Expr> SubsumptionTableEntry::simplifyExistsExpr(ref<Expr> existsExpr,
                                                    bool &hasExistentialsOnly) {
  assert(llvm::isa<ExistsExpr>(existsExpr.get()));

  ref<Expr> body = llvm::dyn_cast<ExistsExpr>(existsExpr.get())->body;
  assert(llvm::isa<AndExpr>(body.get()));

  std::map<ref<Expr>, ref<Expr> > substitution;
  ref<Expr> equalities = getSubstitution(body->getKid(1), substitution);
  ref<Expr> interpolant =
      ApplySubstitutionVisitor(substitution).visit(body->getKid(0));

  ExistsExpr *expr = static_cast<ExistsExpr *>(existsExpr.get());
  if (hasExistentials(expr->variables, equalities)) {
    // we could also replace the occurrence of some variables with its
    // corresponding substitution mapping.
    equalities = ApplySubstitutionVisitor(substitution).visit(equalities);
  }

  ref<Expr> newBody = AndExpr::alloc(interpolant, equalities);

  // FIXME: Need to test the performance of the following.
  if (!hasExistentials(expr->variables, newBody))
    return newBody;

  ref<Expr> ret = simplifyArithmeticBody(existsExpr->rebuild(&newBody),
                                         hasExistentialsOnly);
  return ret;
}

bool SubsumptionTableEntry::subsumed(
    TimingSolver *solver, ExecutionState &state, double timeout,
    const std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
        storedExpressions) {
  // Quick check for subsumption in case the interpolant is empty
  if (empty())
    return true;

  Dependency::ConcreteStore stateConcreteAddressStore = storedExpressions.first;

  Dependency::SymbolicStore stateSymbolicAddressStore =
      storedExpressions.second;

  ref<Expr> stateEqualityConstraints;

  // Build constraints from concrete-address interpolant store
  for (std::vector<llvm::Value *>::iterator
           it1 = concreteAddressStoreKeys.begin(),
           it1End = concreteAddressStoreKeys.end();
       it1 != it1End; ++it1) {
    const Dependency::ConcreteStoreMap lhsConcreteMap =
        concreteAddressStore[*it1];
    const Dependency::ConcreteStoreMap rhsConcreteMap =
        stateConcreteAddressStore[*it1];
    const Dependency::SymbolicStoreMap rhsSymbolicMap =
        stateSymbolicAddressStore[*it1];

    // If the current state does not constrain the same base, subsumption fails.
    if (rhsConcreteMap.empty() && rhsSymbolicMap.empty())
      return false;

    for (Dependency::ConcreteStoreMap::const_iterator
             it2 = lhsConcreteMap.begin(),
             it2End = lhsConcreteMap.end();
         it2 != it2End; ++it2) {

      // The address is not constrained by the current state, therefore
      // the current state is incomparable to the stored interpolant,
      // and we therefore fail the subsumption.
      if (!rhsConcreteMap.count(it2->first))
        return false;

      const Dependency::AddressValuePair rhsConcrete =
          rhsConcreteMap.at(it2->first);
      const ref<Expr> lhsValue = it2->second.second;
      ref<Expr> res;

      if (rhsConcrete.second.get()) {
        // There is a corresponding concrete allocation
        res = EqExpr::alloc(lhsValue, rhsConcrete.second);
      }

      if (!rhsSymbolicMap.empty()) {
        const ref<Expr> lhsConcreteAddress = it2->second.first;
        ref<Expr> conjunction;

        for (Dependency::SymbolicStoreMap::const_iterator
                 it3 = rhsSymbolicMap.begin(),
                 it3End = rhsSymbolicMap.end();
             it3 != it3End; ++it3) {
          // Implication: if lhsConcreteAddress == it3->first, then lhsValue ==
          // it3->second
          ref<Expr> newTerm = OrExpr::alloc(
              EqExpr::alloc(ConstantExpr::alloc(0, Expr::Bool),
                            EqExpr::alloc(lhsConcreteAddress, it3->first)),
              EqExpr::alloc(lhsValue, it3->second));
          if (conjunction.get()) {
            conjunction = AndExpr::alloc(newTerm, conjunction);
          } else {
            conjunction = newTerm;
          }
        }
        // If there were corresponding concrete as well as symbolic allocations
        // in the current state, conjunct them
        res = (res.get() ? AndExpr::alloc(res, conjunction) : conjunction);
      }

      if (res.get()) {
        stateEqualityConstraints =
            (!stateEqualityConstraints.get()
                 ? res
                 : AndExpr::alloc(res, stateEqualityConstraints));
      }
    }
  }

  // Build constraints from symbolic-address interpolant store
  for (std::vector<llvm::Value *>::iterator
           it1 = symbolicAddressStoreKeys.begin(),
           it1End = symbolicAddressStoreKeys.end();
       it1 != it1End; ++it1) {
    const Dependency::SymbolicStoreMap lhsSymbolicMap =
        symbolicAddressStore[*it1];
    const Dependency::ConcreteStoreMap rhsConcreteMap =
        stateConcreteAddressStore[*it1];
    const Dependency::SymbolicStoreMap rhsSymbolicMap =
        stateSymbolicAddressStore[*it1];

    ref<Expr> conjunction;

    for (Dependency::SymbolicStoreMap::const_iterator
             it2 = lhsSymbolicMap.begin(),
             it2End = lhsSymbolicMap.end();
         it2 != it2End; ++it2) {
      ref<Expr> lhsSymbolicAddress = it2->first;
      ref<Expr> lhsValue = it2->second;

      for (Dependency::ConcreteStoreMap::const_iterator
               it3 = rhsConcreteMap.begin(),
               it3End = rhsConcreteMap.end();
           it3 != it3End; ++it3) {
        ref<Expr> rhsConcreteAddress = it3->second.first;
        ref<Expr> rhsValue = it3->second.second;
        // Implication: if lhsSymbolicAddress == rhsConcreteAddress, then
        // lhsValue == rhsValue
        ref<Expr> newTerm =
            OrExpr::alloc(EqExpr::alloc(ConstantExpr::alloc(0, Expr::Bool),
                                        EqExpr::alloc(lhsSymbolicAddress,
                                                      rhsConcreteAddress)),
                          EqExpr::alloc(lhsValue, rhsValue));
        if (conjunction.get()) {
          conjunction = AndExpr::alloc(newTerm, conjunction);
        } else {
          conjunction = newTerm;
        }
      }

      for (Dependency::SymbolicStoreMap::const_iterator
               it3 = rhsSymbolicMap.begin(),
               it3End = rhsSymbolicMap.end();
           it3 != it3End; ++it3) {
        ref<Expr> rhsSymbolicAddress = it3->first;
        ref<Expr> rhsValue = it3->second;
        // Implication: if lhsSymbolicAddress == rhsSymbolicAddress then
        // lhsValue == rhsValue
        ref<Expr> newTerm =
            OrExpr::alloc(EqExpr::alloc(ConstantExpr::alloc(0, Expr::Bool),
                                        EqExpr::alloc(lhsSymbolicAddress,
                                                      rhsSymbolicAddress)),
                          EqExpr::alloc(lhsValue, rhsValue));
        if (conjunction.get()) {
          conjunction = AndExpr::alloc(newTerm, conjunction);
        } else {
          conjunction = newTerm;
        }
      }
    }

    if (conjunction.get()) {
      stateEqualityConstraints =
          (!stateEqualityConstraints.get()
               ? conjunction
               : AndExpr::alloc(conjunction, stateEqualityConstraints));
    }
  }

  Solver::Validity result;
  ref<Expr> query;

  // Here we build the query, after which it is always a conjunction of
  // the interpolant and the state equality constraints.
  if (interpolant.get()) {
    query =
        stateEqualityConstraints.get()
            ? AndExpr::alloc(interpolant, stateEqualityConstraints)
            : AndExpr::alloc(interpolant, ConstantExpr::create(1, Expr::Bool));
  } else if (stateEqualityConstraints.get()) {
    query = AndExpr::alloc(ConstantExpr::create(1, Expr::Bool),
                           stateEqualityConstraints);
  } else {
    // Here both the interpolant constraints and state equality
    // constraints are empty, therefore everything gets subsumed
    return true;
  }

  bool queryHasNoFreeVariables = false;

  if (!existentials.empty()) {
    ref<Expr> existsExpr = ExistsExpr::create(existentials, query);
    // llvm::errs() << "Before simplification:\n";
    // ExprPPrinter::printQuery(llvm::errs(), state.constraints, existsExpr);
    query = simplifyExistsExpr(existsExpr, queryHasNoFreeVariables);
  }

  // If query simplification result was false, we quickly fail without calling
  // the solver
  if (query->isFalse())
    return false;

  bool success = false;

  Z3Solver *z3solver = 0;

  // We call the solver only when the simplified query is
  // not a constant.
  if (!llvm::isa<ConstantExpr>(query)) {
    ++checkSolverCount;

    if (!existentials.empty() && llvm::isa<ExistsExpr>(query)) {
      // llvm::errs() << "Existentials not empty\n";

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

        ref<Expr> falseExpr = ConstantExpr::alloc(0, Expr::Bool);
        constraints.addConstraint(EqExpr::alloc(falseExpr, query->getKid(0)));

        // llvm::errs() << "Querying for satisfiability check:\n";
        // ExprPPrinter::printQuery(llvm::errs(), constraints, falseExpr);

        actualSolverCallTimer.start();
        success = z3solver->getValue(Query(constraints, falseExpr), tmpExpr);
        // double elapsedTime =
        actualSolverCallTimer.stop();

        result = success ? Solver::True : Solver::Unknown;

      } else {
        // llvm::errs() << "Querying for subsumption check:\n";
        // ExprPPrinter::printQuery(llvm::errs(), state.constraints, query);

        actualSolverCallTimer.start();
        success = z3solver->directComputeValidity(
            Query(state.constraints, query), result);
        // double elapsedTime =
        actualSolverCallTimer.stop();

        //        if (elapsedTime > expectedMaxElapsedTime) {
        //            llvm::errs() << "LONG QUERY 2:" << "\n";
        //            Query(state.constraints, query).dump();
        //        }
      }

      z3solver->setCoreSolverTimeout(0);

    } else {
      // llvm::errs() << "No existential\n";

      // llvm::errs() << "Querying for subsumption check:\n";
      // ExprPPrinter::printQuery(llvm::errs(), state.constraints, query);

      // We call the solver in the standard way if the
      // formula is unquantified.

      solver->setTimeout(timeout);
      actualSolverCallTimer.start();
      success = solver->evaluate(state, query, result);
      // double elapsedTime =
      actualSolverCallTimer.stop();

      //      if (elapsedTime > expectedMaxElapsedTime) {
      //          llvm::errs() << "LONG QUERY 3:" << "\n";
      //          Query(state.constraints, query).dump();
      //      }

      solver->setTimeout(0);
    }
  } else {
    if (query->isTrue())
      return true;
    return false;
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

    // State subsumed, we mark needed constraints on the
    // path condition.

    // We create path condition marking structure to mark core constraints
    state.itreeNode->unsatCoreMarking(unsatCore, state);
    return true;
  }
    // Here the solver could not decide that the subsumption is valid.
    // It may have decided invalidity, however,
    // CexCachingSolver::computeValidity,
    // which was eventually called from solver->evaluate
    // is conservative, where it returns Solver::Unknown even in case when
    // invalidity is established by the solver.
    // llvm::errs() << "Solver did not decide validity\n";

    ++checkSolverFailureCount;
    if (z3solver)
      delete z3solver;

    return false;
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

  if (!concreteAddressStore.empty()) {
    stream << "allocations = [";
    for (Dependency::ConcreteStore::const_iterator
             it1Begin = concreteAddressStore.begin(),
             it1End = concreteAddressStore.end(), it1 = it1Begin;
         it1 != it1End; ++it1) {
      for (Dependency::ConcreteStoreMap::const_iterator
               it2Begin = it1->second.begin(),
               it2End = it1->second.end(), it2 = it2Begin;
           it2 != it2End; ++it2) {
        if (it1 != it1Begin || it2 != it2Begin)
          stream << ",";
        stream << "(";
        it2->second.first->print(stream);
        stream << ",";
        it2->second.second->print(stream);
        stream << ")";
      }
    }
    stream << "]\n";
  }

  if (!existentials.empty()) {
    stream << "existentials = [";
    for (std::set<const Array *>::const_iterator itBegin = existentials.begin(),
                                                 itEnd = existentials.end(),
                                                 it = itBegin;
         it != itEnd; ++it) {
      if (it != itBegin)
        stream << ", ";
      stream << (*it)->name;
    }
    stream << "]\n";
  }
}

void SubsumptionTableEntry::printStat(llvm::raw_ostream &stream) {
  stream << "KLEE: done:     Time for actual solver calls in subsumption check "
            "(ms) = " << actualSolverCallTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     Number of solver calls for subsumption check "
            "(failed) = " << checkSolverCount << " (" << checkSolverFailureCount
         << ")\n";
}

/**/

StatTimer ITree::setCurrentINodeTimer;
StatTimer ITree::removeTimer;
StatTimer ITree::subsumptionCheckTimer;
StatTimer ITree::markPathConditionTimer;
StatTimer ITree::splitTimer;
StatTimer ITree::executeOnNodeTimer;

unsigned long ITree::subsumptionCheckCount = 0;

void ITree::printTimeStat(llvm::raw_ostream &stream) {
  stream << "KLEE: done:     setCurrentINode = " << setCurrentINodeTimer.get() *
                                                        1000 << "\n";
  stream << "KLEE: done:     remove = " << removeTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     subsumptionCheckTimer = "
         << subsumptionCheckTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     markPathCondition = "
         << markPathConditionTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     split = " << splitTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     executeOnNode = " << executeOnNodeTimer.get() *
                                                      1000 << "\n";
}

void ITree::printTableStat(llvm::raw_ostream &stream) const {
  double programPointNumber = 0.0, entryNumber = 0.0;
  for (std::map<uintptr_t,
                std::vector<SubsumptionTableEntry *> >::const_iterator
           it = subsumptionTable.begin(),
           itEnd = subsumptionTable.end();
       it != itEnd; ++it) {
    if (!it->second.empty()) {
      entryNumber += it->second.size();
      ++programPointNumber;
    }
  }

  SubsumptionTableEntry::printStat(stream);

  stream
      << "KLEE: done:     Average table entries per subsumption checkpoint = "
      << StatTimer::inTwoDecimalPoints(entryNumber / programPointNumber)
      << "\n";

  stream << "KLEE: done:     Average solver calls per subsumption check = "
         << StatTimer::inTwoDecimalPoints(
                (double)SubsumptionTableEntry::checkSolverCount /
                (double)subsumptionCheckCount) << "\n";
}

void ITree::dumpInterpolationStat() {
  bool useColors = llvm::errs().is_displayed();
  if (useColors)
    llvm::errs().changeColor(llvm::raw_ostream::GREEN,
                             /*bold=*/true,
                             /*bg=*/false);
  llvm::errs() << "\nKLEE: done: Subsumption statistics\n";
  printTableStat(llvm::errs());
  llvm::errs() << "KLEE: done: ITree method execution times (ms):\n";
  printTimeStat(llvm::errs());
  llvm::errs() << "KLEE: done: ITreeNode method execution times (ms):\n";
  ITreeNode::printTimeStat(llvm::errs());
  if (useColors)
    llvm::errs().resetColor();
}

ITree::ITree(ExecutionState *_root) {
  currentINode = 0;
  if (!_root->itreeNode) {
    currentINode = new ITreeNode(0);
  }
  root = currentINode;
}

ITree::~ITree() {
  for (std::map<uintptr_t, std::vector<SubsumptionTableEntry *> >::iterator
           it = subsumptionTable.begin(),
           itEnd = subsumptionTable.end();
       it != itEnd; ++it) {
    for (std::vector<SubsumptionTableEntry *>::iterator
             it1 = it->second.begin(),
             it1End = it->second.end();
         it1 != it1End; ++it1) {
      delete *it1;
    }
    it->second.clear();
  }
  subsumptionTable.clear();
}

bool ITree::subsumptionCheck(TimingSolver *solver, ExecutionState &state,
                             double timeout) {
  subsumptionCheckCount++; // For profiling

  assert(state.itreeNode == currentINode);

  // Immediately return if the state's instruction is not the
  // the interpolation node id. The interpolation node id is the
  // first instruction executed of the sequence executed for a state
  // node, typically this the first instruction of a basic block.
  // Subsumption check only matches against this first instruction.
  if (!state.itreeNode || reinterpret_cast<uintptr_t>(state.pc->inst) !=
                              state.itreeNode->getNodeId())
    return false;

  subsumptionCheckTimer.start();
  std::vector<SubsumptionTableEntry *> entryList =
      subsumptionTable[state.itreeNode->getNodeId()];

  if (entryList.empty())
    return false;

  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
  storedExpressions = state.itreeNode->getStoredExpressions();

  for (std::vector<SubsumptionTableEntry *>::iterator it = entryList.begin(),
                                                      itEnd = entryList.end();
       it != itEnd; ++it) {

    if ((*it)->subsumed(solver, state, timeout, storedExpressions)) {
      // We mark as subsumed such that the node will not be
      // stored into table (the table already contains a more
      // general entry).
      currentINode->isSubsumed = true;

      // Mark the node as subsumed, and create a subsumption edge
      SearchTree::markAsSubsumed(currentINode, (*it));
      subsumptionCheckTimer.stop();
      return true;
    }
  }
  subsumptionCheckTimer.stop();
  return false;
}

void ITree::store(SubsumptionTableEntry *subItem) {
  subsumptionTable[subItem->nodeId].push_back(subItem);
}

void ITree::setCurrentINode(ExecutionState &state, uintptr_t programPoint) {
  setCurrentINodeTimer.start();
  currentINode = state.itreeNode;
  currentINode->setNodeLocation(programPoint);
  SearchTree::setCurrentNode(state, programPoint);
  setCurrentINodeTimer.stop();
}

void ITree::remove(ITreeNode *node) {
  removeTimer.start();
  assert(!node->left && !node->right);
  do {
    ITreeNode *p = node->parent;

    // As the node is about to be deleted, it must have been completely
    // traversed, hence the correct time to table the interpolant.
    if (!node->isSubsumed) {
      SubsumptionTableEntry *entry = new SubsumptionTableEntry(node);
      store(entry);
      SearchTree::addTableEntryMapping(node, entry);
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
  removeTimer.stop();
}

std::pair<ITreeNode *, ITreeNode *>
ITree::split(ITreeNode *parent, ExecutionState *left, ExecutionState *right) {
  splitTimer.start();
  parent->split(left, right);
  SearchTree::addChildren(parent, parent->left, parent->right);
  std::pair<ITreeNode *, ITreeNode *> ret(parent->left, parent->right);
  splitTimer.stop();
  return ret;
}

void ITree::markPathCondition(ExecutionState &state, TimingSolver *solver) {
  markPathConditionTimer.start();
  std::vector<ref<Expr> > unsatCore = solver->getUnsatCore();

  AllocationGraph *g = new AllocationGraph();

  llvm::BranchInst *binst =
      llvm::dyn_cast<llvm::BranchInst>(state.prevPC->inst);
  if (binst) {
    currentINode->dependency->markAllValues(g, binst->getCondition());
  }

  PathCondition *pc = currentINode->pathCondition;

  if (pc != 0) {
    for (std::vector<ref<Expr> >::iterator it = unsatCore.begin(),
                                           itEnd = unsatCore.end();
         it != itEnd; ++it) {
      for (; pc != 0; pc = pc->cdr()) {
        if (pc->car().compare(it->get()) == 0) {
          pc->setAsCore(g);
          pc = pc->cdr();
          break;
        }
      }
    }
  }

  // llvm::errs() << "AllocationGraph\n";
  // g->dump();

  // Compute memory allocations needed by the unsatisfiability core
  currentINode->computeCoreAllocations(g);

  delete g; // Delete the AllocationGraph object
  markPathConditionTimer.stop();
}

void ITree::execute(llvm::Instruction *instr) {
  std::vector<ref<Expr> > dummyArgs;
  executeOnNode(currentINode, instr, dummyArgs);
}

void ITree::execute(llvm::Instruction *instr, ref<Expr> arg1) {
  std::vector<ref<Expr> > args;
  args.push_back(arg1);
  executeOnNode(currentINode, instr, args);
}

void ITree::execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2) {
  std::vector<ref<Expr> > args;
  args.push_back(arg1);
  args.push_back(arg2);
  executeOnNode(currentINode, instr, args);
}

void ITree::execute(llvm::Instruction *instr, ref<Expr> arg1, ref<Expr> arg2,
                    ref<Expr> arg3) {
  std::vector<ref<Expr> > args;
  args.push_back(arg1);
  args.push_back(arg2);
  args.push_back(arg3);
  executeOnNode(currentINode, instr, args);
}

void ITree::execute(llvm::Instruction *instr, std::vector<ref<Expr> > &args) {
  executeOnNode(currentINode, instr, args);
}

void ITree::executeOnNode(ITreeNode *node, llvm::Instruction *instr,
                          std::vector<ref<Expr> > &args) {
  executeOnNodeTimer.start();
  node->execute(instr, args);
  executeOnNodeTimer.stop();
}

void ITree::printNode(llvm::raw_ostream &stream, ITreeNode *n,
                      std::string edges) const {
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

void ITree::print(llvm::raw_ostream &stream) {
  stream << "------------------------- ITree Structure "
            "---------------------------\n";
  stream << this->root->nodeId;
  if (this->root == this->currentINode) {
    stream << " (active)";
  }
  this->printNode(stream, this->root, "");
  stream << "\n------------------------- Subsumption Table "
            "-------------------------\n";
  for (std::map<uintptr_t, std::vector<SubsumptionTableEntry *> >::iterator
           it = subsumptionTable.begin(),
           itEnd = subsumptionTable.end();
       it != itEnd; ++it) {
    for (std::vector<SubsumptionTableEntry *>::iterator
             it1 = it->second.begin(),
             it1End = it->second.end();
         it1 != it1End; ++it1) {
      (*it1)->print(stream);
    }
  }
}

void ITree::dump() { this->print(llvm::errs()); }

/**/

// Statistics
StatTimer ITreeNode::getInterpolantTimer;
StatTimer ITreeNode::addConstraintTimer;
StatTimer ITreeNode::splitTimer;
StatTimer ITreeNode::makeMarkerMapTimer;
StatTimer ITreeNode::deleteMarkerMapTimer;
StatTimer ITreeNode::executeTimer;
StatTimer ITreeNode::bindCallArgumentsTimer;
StatTimer ITreeNode::popAbstractDependencyFrameTimer;
StatTimer ITreeNode::getStoredExpressionsTimer;
StatTimer ITreeNode::getStoredCoreExpressionsTimer;
StatTimer ITreeNode::computeCoreAllocationsTimer;

void ITreeNode::printTimeStat(llvm::raw_ostream &stream) {
  stream << "KLEE: done:     getInterpolant = " << getInterpolantTimer.get() *
                                                       1000 << "\n";
  stream << "KLEE: done:     addConstraintTime = " << addConstraintTimer.get() *
                                                          1000 << "\n";
  stream << "KLEE: done:     splitTime = " << splitTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     makeMarkerMap = " << makeMarkerMapTimer.get() *
                                                      1000 << "\n";
  stream << "KLEE: done:     deleteMarkerMap = " << deleteMarkerMapTimer.get() *
                                                        1000 << "\n";
  stream << "KLEE: done:     execute = " << executeTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     bindCallArguments = "
         << bindCallArgumentsTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     popAbstractDependencyFrame = "
         << popAbstractDependencyFrameTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     getStoredExpressions = "
         << getStoredExpressionsTimer.get() * 1000 << "\n";
  stream << "KLEE: done:     getStoredCoreExpressions = "
         << getStoredCoreExpressionsTimer.get() << "\n";
  stream << "KLEE: done:     computeCoreAllocations = "
         << computeCoreAllocationsTimer.get() * 1000 << "\n";
}

ITreeNode::ITreeNode(ITreeNode *_parent)
    : parent(_parent), left(0), right(0), nodeId(0), isSubsumed(false),
      graph(_parent ? _parent->graph : 0) {

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
ITreeNode::getInterpolant(std::set<const Array *> &replacements) const {
  ITreeNode::getInterpolantTimer.start();
  ref<Expr> expr = this->pathCondition->packInterpolant(replacements);
  ITreeNode::getInterpolantTimer.stop();
  return expr;
}

void ITreeNode::addConstraint(ref<Expr> &constraint, llvm::Value *condition) {
  ITreeNode::addConstraintTimer.start();
  pathCondition =
      new PathCondition(constraint, dependency, condition, pathCondition);
  graph->addPathCondition(this, pathCondition, constraint);
  ITreeNode::addConstraintTimer.stop();
}

void ITreeNode::split(ExecutionState *leftData, ExecutionState *rightData) {
  ITreeNode::splitTimer.start();
  assert(left == 0 && right == 0);
  leftData->itreeNode = left = new ITreeNode(this);
  rightData->itreeNode = right = new ITreeNode(this);
  ITreeNode::splitTimer.stop();
}

void ITreeNode::execute(llvm::Instruction *instr,
                        std::vector<ref<Expr> > &args) {
  executeTimer.start();
  dependency->execute(instr, args);
  executeTimer.stop();
}

void ITreeNode::bindCallArguments(llvm::Instruction *site,
                                  std::vector<ref<Expr> > &arguments) {
  ITreeNode::bindCallArgumentsTimer.start();
  dependency->bindCallArguments(site, arguments);
  ITreeNode::bindCallArgumentsTimer.stop();
}

void ITreeNode::popAbstractDependencyFrame(llvm::CallInst *site,
                                           llvm::Instruction *inst,
                                           ref<Expr> returnValue) {
  // TODO: This is probably where we should simplify
  // the dependency graph by removing callee values.
  ITreeNode::popAbstractDependencyFrameTimer.start();
  dependency->bindReturnValue(site, inst, returnValue);
  ITreeNode::popAbstractDependencyFrameTimer.stop();
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
ITreeNode::getStoredExpressions() const {
  ITreeNode::getStoredExpressionsTimer.start();
  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore> ret;
  std::set<const Array *> dummyReplacements;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret = parent->dependency->getStoredExpressions(dummyReplacements, false);
  ITreeNode::getStoredExpressionsTimer.stop();
  return ret;
}

std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore>
ITreeNode::getStoredCoreExpressions(std::set<const Array *> &replacements)
    const {
  ITreeNode::getStoredCoreExpressionsTimer.start();
  std::pair<Dependency::ConcreteStore, Dependency::SymbolicStore> ret;

  // Since a program point index is a first statement in a basic block,
  // the allocations to be stored in subsumption table should be obtained
  // from the parent node.
  if (parent)
    ret = parent->dependency->getStoredExpressions(replacements, true);
  ITreeNode::getStoredCoreExpressionsTimer.stop();
  return ret;
}

void ITreeNode::unsatCoreMarking(std::vector<ref<Expr> > unsatCore,
                                 ExecutionState &state) {
  // State subsumed, we mark needed constraints on the
  // path condition.
  // We create path condition marking structure to mark core constraints
  ITreeNode::makeMarkerMapTimer.start();
  std::map<Expr *, PathCondition *> markerMap;
  for (PathCondition *it = pathCondition; it != 0; it = it->cdr()) {
    if (llvm::isa<OrExpr>(it->car().get())) {
      // FIXME: Break up disjunction into its components, because each disjunct
      // is solved separately. The or constraint was due to state merge.
      // Hence, the following is just a makeshift for when state merge is
      // properly implemented.
      markerMap[it->car()->getKid(0).get()] = it;
      markerMap[it->car()->getKid(1).get()] = it;
    }
    markerMap[it->car().get()] = it;
  }
  ITreeNode::makeMarkerMapTimer.stop();

  AllocationGraph *g = new AllocationGraph();
  for (std::vector<ref<Expr> >::iterator it1 = unsatCore.begin();
       it1 != unsatCore.end(); it1++) {
    // FIXME: Sometimes some constraints are not in the PC. This is
    // because constraints are not properly added at state merge.
    PathCondition *cond = markerMap[it1->get()];
    if (cond)
      cond->setAsCore(g);
  }
  // llvm::errs() << "AllocationGraph\n";
  // g->dump();
  // We mark memory allocations needed for the unsatisfiabilty core
  computeCoreAllocations(g);
  delete g; // Delete the AllocationGraph object
}

void ITreeNode::computeCoreAllocations(AllocationGraph *g) {
  ITreeNode::computeCoreAllocationsTimer.start();
  dependency->computeCoreAllocations(g);
  ITreeNode::computeCoreAllocationsTimer.stop();
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
