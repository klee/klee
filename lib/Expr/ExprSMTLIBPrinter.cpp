//===-- ExprSMTLIBPrinter.cpp -----------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "klee/util/ExprSMTLIBPrinter.h"

#include <stack>

namespace ExprSMTLIBOptions {
// Command line options
llvm::cl::opt<klee::ExprSMTLIBPrinter::ConstantDisplayMode>
argConstantDisplayMode(
    "smtlib-display-constants",
    llvm::cl::desc("Sets how bitvector constants are written in generated "
                   "SMT-LIBv2 files (default=dec)"),
    llvm::cl::values(clEnumValN(klee::ExprSMTLIBPrinter::BINARY, "bin",
                                "Use binary form (e.g. #b00101101)"),
                     clEnumValN(klee::ExprSMTLIBPrinter::HEX, "hex",
                                "Use Hexadecimal form (e.g. #x2D)"),
                     clEnumValN(klee::ExprSMTLIBPrinter::DECIMAL, "dec",
                                "Use decimal form (e.g. (_ bv45 8) )"),
                     clEnumValEnd),
    llvm::cl::init(klee::ExprSMTLIBPrinter::DECIMAL));

llvm::cl::opt<bool> humanReadableSMTLIB(
    "smtlib-human-readable",
    llvm::cl::desc(
        "Enables generated SMT-LIBv2 files to be human readable (default=off)"),
    llvm::cl::init(false));

llvm::cl::opt<klee::ExprSMTLIBPrinter::AbbreviationMode> abbreviationMode(
    "smtlib-abbreviation-mode",
    llvm::cl::desc(
        "Choose abbreviation mode to use in SMT-LIBv2 files (default=let)"),
    llvm::cl::values(clEnumValN(klee::ExprSMTLIBPrinter::ABBR_NONE, "none",
                                "Do not abbreviate"),
                     clEnumValN(klee::ExprSMTLIBPrinter::ABBR_LET, "let",
                                "Abbreviate with let"),
                     clEnumValN(klee::ExprSMTLIBPrinter::ABBR_NAMED, "named",
                                "Abbreviate with :named annotations"),
                     clEnumValEnd),
    llvm::cl::init(klee::ExprSMTLIBPrinter::ABBR_LET));
}

namespace klee {

ExprSMTLIBPrinter::ExprSMTLIBPrinter()
    : usedArrays(), o(NULL), query(NULL), p(NULL), haveConstantArray(false),
      logicToUse(QF_AUFBV),
      humanReadable(ExprSMTLIBOptions::humanReadableSMTLIB),
      smtlibBoolOptions(), arraysToCallGetValueOn(NULL) {
  setConstantDisplayMode(ExprSMTLIBOptions::argConstantDisplayMode);
  setAbbreviationMode(ExprSMTLIBOptions::abbreviationMode);
}

ExprSMTLIBPrinter::~ExprSMTLIBPrinter() {
  if (p != NULL)
    delete p;
}

void ExprSMTLIBPrinter::setOutput(llvm::raw_ostream &output) {
  o = &output;
  if (p != NULL)
    delete p;

  p = new PrintContext(output);
}

void ExprSMTLIBPrinter::setQuery(const Query &q) {
  query = &q;
  reset(); // clear the data structures
  scanAll();
}

void ExprSMTLIBPrinter::reset() {
  bindings.clear();
  orderedBindings.clear();
  seenExprs.clear();
  usedArrays.clear();
  haveConstantArray = false;

  /* Clear the PRODUCE_MODELS option if it was automatically set.
   * We need to do this because the next query might not need the
   * (get-value) SMT-LIBv2 command.
   */
  if (arraysToCallGetValueOn != NULL)
    setSMTLIBboolOption(PRODUCE_MODELS, OPTION_DEFAULT);

  arraysToCallGetValueOn = NULL;
}

bool ExprSMTLIBPrinter::isHumanReadable() { return humanReadable; }

bool ExprSMTLIBPrinter::setConstantDisplayMode(ConstantDisplayMode cdm) {
  if (cdm > DECIMAL)
    return false;

  this->cdm = cdm;
  return true;
}

void ExprSMTLIBPrinter::printConstant(const ref<ConstantExpr> &e) {
  /* Handle simple boolean constants */

  if (e->isTrue()) {
    *p << "true";
    return;
  }

  if (e->isFalse()) {
    *p << "false";
    return;
  }

  /* Handle bitvector constants */

  std::string value;

  /* SMTLIBv2 deduces the bit-width (should be 8-bits in our case)
   * from the length of the string (e.g. zero is #b00000000). LLVM
   * doesn't know about this so we need to pad the printed output
   * with the appropriate number of zeros (zeroPad)
   */
  unsigned int zeroPad = 0;

  switch (cdm) {
  case BINARY:
    e->toString(value, 2);
    *p << "#b";

    zeroPad = e->getWidth() - value.length();

    for (unsigned int count = 0; count < zeroPad; count++)
      *p << "0";

    *p << value;
    break;

  case HEX:
    e->toString(value, 16);
    *p << "#x";

    zeroPad = (e->getWidth() / 4) - value.length();
    for (unsigned int count = 0; count < zeroPad; count++)
      *p << "0";

    *p << value;
    break;

  case DECIMAL:
    e->toString(value, 10);
    *p << "(_ bv" << value << " " << e->getWidth() << ")";
    break;

  default:
    llvm_unreachable("Unexpected constant display mode");
  }
}

void ExprSMTLIBPrinter::printExpression(
    const ref<Expr> &e, ExprSMTLIBPrinter::SMTLIB_SORT expectedSort) {
  // check if casting might be necessary
  if (getSort(e) != expectedSort) {
    printCastToSort(e, expectedSort);
    return;
  }

  switch (abbrMode) {
  case ABBR_NONE:
    break;

  case ABBR_LET: {
    BindingMap::iterator i = bindings.find(e);
    if (i != bindings.end()) {
      *p << "?B" << i->second;
      return;
    }
    break;
  }

  case ABBR_NAMED: {
    BindingMap::iterator i = bindings.find(e);
    if (i != bindings.end()) {
      if (i->second > 0) {
        *p << "(! ";
        printFullExpression(e, expectedSort);
        *p << " :named ?B" << i->second << ")";
        i->second = -i->second;
      } else {
        *p << "?B" << -i->second;
      }
      return;
    }
    break;
  }
  }

  printFullExpression(e, expectedSort);
}

void ExprSMTLIBPrinter::printFullExpression(
    const ref<Expr> &e, ExprSMTLIBPrinter::SMTLIB_SORT expectedSort) {
  switch (e->getKind()) {
  case Expr::Constant:
    printConstant(cast<ConstantExpr>(e));
    return; // base case

  case Expr::NotOptimized:
    // skip to child
    printExpression(e->getKid(0), expectedSort);
    return;

  case Expr::Read:
    printReadExpr(cast<ReadExpr>(e));
    return;

  case Expr::Extract:
    printExtractExpr(cast<ExtractExpr>(e));
    return;

  case Expr::SExt:
  case Expr::ZExt:
    printCastExpr(cast<CastExpr>(e));
    return;

  case Expr::Select:
    // the if-then-else expression.
    printSelectExpr(cast<SelectExpr>(e), expectedSort);
    return;

  case Expr::Eq:
  case Expr::Ne:
    /* The "=" and distinct operators are special in that it can take any sort
     * but we must enforce that both arguments are the same sort. We do this in
     * a lazy way by enforcing the second argument is of the same type as the
     * first.
     */
    printSortArgsExpr(e, getSort(e->getKid(0)));
    return;

  case Expr::And:
  case Expr::Or:
  case Expr::Xor:
  case Expr::Not:
    /* These operators have a bitvector version and a bool version.
     * For these operators only (e.g. wouldn't apply to bvult) if the expected
     * sort of the expression is T then that implies the arguments are also of
     * type T.
     */
    printLogicalOrBitVectorExpr(e, expectedSort);
    return;

  case Expr::AShr:
    printAShrExpr(cast<AShrExpr>(e));
    return;

  default:
    /* The remaining operators (Add,Sub...,Ult,Ule,..)
     * Expect SORT_BITVECTOR arguments
     */
    printSortArgsExpr(e, SORT_BITVECTOR);
    return;
  }
}

void ExprSMTLIBPrinter::printReadExpr(const ref<ReadExpr> &e) {
  *p << "(" << getSMTLIBKeyword(e) << " ";
  p->pushIndent();

  printSeperator();

  // print array with updates recursively
  printUpdatesAndArray(e->updates.head, e->updates.root);

  // print index
  printSeperator();
  printExpression(e->index, SORT_BITVECTOR);

  p->popIndent();
  printSeperator();
  *p << ")";
}

void ExprSMTLIBPrinter::printExtractExpr(const ref<ExtractExpr> &e) {
  unsigned int lowIndex = e->offset;
  unsigned int highIndex = lowIndex + e->width - 1;

  *p << "((_ " << getSMTLIBKeyword(e) << " " << highIndex << "  " << lowIndex
     << ") ";

  p->pushIndent(); // add indent for recursive call
  printSeperator();

  // recurse
  printExpression(e->getKid(0), SORT_BITVECTOR);

  p->popIndent(); // pop indent added for the recursive call
  printSeperator();
  *p << ")";
}

void ExprSMTLIBPrinter::printCastExpr(const ref<CastExpr> &e) {
  /* sign_extend and zero_extend behave slightly unusually in SMTLIBv2,
   * instead of specifying of what bit-width we would like to extend to
   * we specify how many bits to add to the child expression
   *
   * e.g
   * ((_ sign_extend 64) (_ bv5 32))
   *
   * gives a (_ BitVec 96) instead of (_ BitVec 64)
   *
   * So we must work out how many bits we need to add.
   *
   * (e->width) is the desired number of bits
   * (e->src->getWidth()) is the number of bits in the child
   */
  unsigned int numExtraBits = (e->width) - (e->src->getWidth());

  *p << "((_ " << getSMTLIBKeyword(e) << " " << numExtraBits << ") ";

  p->pushIndent(); // add indent for recursive call
  printSeperator();

  // recurse
  printExpression(e->src, SORT_BITVECTOR);

  p->popIndent(); // pop indent added for recursive call
  printSeperator();

  *p << ")";
}

void ExprSMTLIBPrinter::printAShrExpr(const ref<AShrExpr> &e) {
  // There is a difference between AShr and SMT-LIBv2's
  // bvashr function when the shift amount is >= the bit width
  // so we need to treat it specially here.
  //
  // Technically this is undefined behaviour for LLVM's ashr operator
  // but currently llvm::APInt:ashr(...) will emit 0 if the shift amount
  // is >= the bit width but this does not match how SMT-LIBv2's bvashr
  // behaves as demonstrates by the following query
  //
  // (declare-fun x () (_ BitVec 32))
  // (declare-fun y () (_ BitVec 32))
  // (declare-fun result () (_ BitVec 32))
  // (assert (bvuge y (_ bv32 32)))
  // (assert (= result (bvashr x y)))
  // (assert (distinct (_ bv0 32) result))
  // (check-sat)
  // sat
  //
  // Our solution is to instead emit
  //
  // (ite (bvuge shift_amount bit_width)
  //      (_ bv0 bitwidth)
  //      (bvashr value_to_shift shift_amount)
  // )
  //

  Expr::Width bitWidth = e->getKid(0)->getWidth();
  assert(bitWidth > 0 && "Invalid bit width");
  ref<Expr> bitWidthExpr = ConstantExpr::create(bitWidth, bitWidth);
  ref<Expr> zeroExpr = ConstantExpr::create(0, bitWidth);

  // FIXME: we print e->getKid(1) twice and it might not get
  // abbreviated
  *p << "(ite";
  p->pushIndent();
  printSeperator();

  *p << "(bvuge";
  p->pushIndent();
  printSeperator();
  printExpression(e->getKid(1), SORT_BITVECTOR);
  printSeperator();
  printExpression(bitWidthExpr, SORT_BITVECTOR);
  p->popIndent();
  printSeperator();
  *p << ")";

  printSeperator();
  printExpression(zeroExpr, SORT_BITVECTOR);
  printSeperator();

  *p << "(bvashr";
  p->pushIndent();
  printSeperator();
  printExpression(e->getKid(0), SORT_BITVECTOR);
  printSeperator();
  printExpression(e->getKid(1), SORT_BITVECTOR);
  p->popIndent();
  printSeperator();
  *p << ")";

  p->popIndent();
  printSeperator();
  *p << ")";
}

const char *ExprSMTLIBPrinter::getSMTLIBKeyword(const ref<Expr> &e) {

  switch (e->getKind()) {
  case Expr::Read:
    return "select";
  case Expr::Select:
    return "ite";
  case Expr::Concat:
    return "concat";
  case Expr::Extract:
    return "extract";
  case Expr::ZExt:
    return "zero_extend";
  case Expr::SExt:
    return "sign_extend";

  case Expr::Add:
    return "bvadd";
  case Expr::Sub:
    return "bvsub";
  case Expr::Mul:
    return "bvmul";
  case Expr::UDiv:
    return "bvudiv";
  case Expr::SDiv:
    return "bvsdiv";
  case Expr::URem:
    return "bvurem";
  case Expr::SRem:
    return "bvsrem";

  /* And, Xor, Not and Or are not handled here because there different versions
   * for different sorts. See printLogicalOrBitVectorExpr()
   */

  case Expr::Shl:
    return "bvshl";
  case Expr::LShr:
    return "bvlshr";
  // AShr is not supported here. See printAShrExpr()

  case Expr::Eq:
    return "=";
  case Expr::Ne:
    return "distinct";

  case Expr::Ult:
    return "bvult";
  case Expr::Ule:
    return "bvule";
  case Expr::Ugt:
    return "bvugt";
  case Expr::Uge:
    return "bvuge";

  case Expr::Slt:
    return "bvslt";
  case Expr::Sle:
    return "bvsle";
  case Expr::Sgt:
    return "bvsgt";
  case Expr::Sge:
    return "bvsge";

  default:
    llvm_unreachable("Conversion from Expr to SMTLIB keyword failed");
  }
}

void ExprSMTLIBPrinter::printUpdatesAndArray(const UpdateNode *un,
                                             const Array *root) {
  if (un != NULL) {
    *p << "(store ";
    p->pushIndent();
    printSeperator();

    // recurse to get the array or update that this store operations applies to
    printUpdatesAndArray(un->next, root);

    printSeperator();

    // print index
    printExpression(un->index, SORT_BITVECTOR);
    printSeperator();

    // print value that is assigned to this index of the array
    printExpression(un->value, SORT_BITVECTOR);

    p->popIndent();
    printSeperator();
    *p << ")";
  } else {
    // The base case of the recursion
    *p << root->name;
  }
}

void ExprSMTLIBPrinter::scanAll() {
  // perform scan of all expressions
  for (ConstraintManager::const_iterator i = query->constraints.begin();
       i != query->constraints.end(); i++)
    scan(*i);

  // Scan the query too
  scan(query->expr);

  // Scan bindings for expression intra-dependencies
  if (abbrMode == ABBR_LET)
    scanBindingExprDeps();
}

void ExprSMTLIBPrinter::generateOutput() {
  if (p == NULL || query == NULL || o == NULL) {
    llvm::errs() << "ExprSMTLIBPrinter::generateOutput() Can't print SMTLIBv2. "
                    "Output or query bad!\n";
    return;
  }

  if (humanReadable)
    printNotice();
  printOptions();
  printSetLogic();
  printArrayDeclarations();

  if (humanReadable)
    printHumanReadableQuery();
  else
    printMachineReadableQuery();

  printAction();
  printExit();
}

void ExprSMTLIBPrinter::printSetLogic() {
  *o << "(set-logic ";
  switch (logicToUse) {
  case QF_ABV:
    *o << "QF_ABV";
    break;
  case QF_AUFBV:
    *o << "QF_AUFBV";
    break;
  }
  *o << " )\n";
}

namespace {

struct ArrayPtrsByName {
  bool operator()(const Array *a1, const Array *a2) const {
    return a1->name < a2->name;
  }
};

}

void ExprSMTLIBPrinter::printArrayDeclarations() {
  // Assume scan() has been called
  if (humanReadable)
    *o << "; Array declarations\n";

  // Declare arrays in a deterministic order.
  std::vector<const Array *> sortedArrays(usedArrays.begin(), usedArrays.end());
  std::sort(sortedArrays.begin(), sortedArrays.end(), ArrayPtrsByName());
  for (std::vector<const Array *>::iterator it = sortedArrays.begin();
       it != sortedArrays.end(); it++) {
    *o << "(declare-fun " << (*it)->name << " () "
                                            "(Array (_ BitVec "
       << (*it)->getDomain() << ") "
                                "(_ BitVec " << (*it)->getRange() << ") ) )"
       << "\n";
  }

  // Set array values for constant values
  if (haveConstantArray) {
    if (humanReadable)
      *o << "; Constant Array Definitions\n";

    const Array *array;

    // loop over found arrays
    for (std::vector<const Array *>::iterator it = sortedArrays.begin();
         it != sortedArrays.end(); it++) {
      array = *it;
      int byteIndex = 0;
      if (array->isConstantArray()) {
        /*loop over elements in the array and generate an assert statement
          for each one
         */
        for (std::vector<ref<ConstantExpr> >::const_iterator
                 ce = array->constantValues.begin();
             ce != array->constantValues.end(); ce++, byteIndex++) {
          *p << "(assert (";
          p->pushIndent();
          *p << "= ";
          p->pushIndent();
          printSeperator();

          *p << "(select " << array->name << " (_ bv" << byteIndex << " "
             << array->getDomain() << ") )";
          printSeperator();
          printConstant((*ce));

          p->popIndent();
          printSeperator();
          *p << ")";
          p->popIndent();
          printSeperator();
          *p << ")";

          p->breakLineI();
        }
      }
    }
  }
}

void ExprSMTLIBPrinter::printHumanReadableQuery() {
  assert(humanReadable && "method must be called in humanReadable mode");
  *o << "; Constraints\n";

  if (abbrMode != ABBR_LET) {
    // Generate assert statements for each constraint
    for (ConstraintManager::const_iterator i = query->constraints.begin();
         i != query->constraints.end(); i++) {
      printAssert(*i);
    }

    *o << "; QueryExpr\n";

    // We negate the Query Expr because in KLEE queries are solved
    // in terms of validity, but SMT-LIB works in terms of satisfiability
    ref<Expr> queryAssert = Expr::createIsZero(query->expr);
    printAssert(queryAssert);
  } else {
    // let bindings are only scoped within a single (assert ...) so
    // the entire query must be printed within a single assert
    *o << "; Constraints and QueryExpr\n";
    printQueryInSingleAssert();
  }
}
void ExprSMTLIBPrinter::printMachineReadableQuery() {
  assert(!humanReadable && "method should not be called in humanReadable mode");
  printQueryInSingleAssert();
}


void ExprSMTLIBPrinter::printQueryInSingleAssert() {
  // We negate the Query Expr because in KLEE queries are solved
  // in terms of validity, but SMT-LIB works in terms of satisfiability
  ref<Expr> queryAssert = Expr::createIsZero(query->expr);

  // Print constraints inside the main query to reuse the Expr bindings
  for (std::vector<ref<Expr> >::const_iterator i = query->constraints.begin(),
                                               e = query->constraints.end();
       i != e; ++i) {
    queryAssert = AndExpr::create(queryAssert, *i);
  }

  // print just a single (assert ...) containing entire query
  printAssert(queryAssert);
}

void ExprSMTLIBPrinter::printAction() {
  // Ask solver to check for satisfiability
  *o << "(check-sat)\n";

  /* If we have arrays to find the values of then we'll
   * ask the solver for the value of each bitvector in each array
   */
  if (arraysToCallGetValueOn != NULL && !arraysToCallGetValueOn->empty()) {

    const Array *theArray = 0;

    // loop over the array names
    for (std::vector<const Array *>::const_iterator it =
             arraysToCallGetValueOn->begin();
         it != arraysToCallGetValueOn->end(); it++) {
      theArray = *it;
      // Loop over the array indices
      for (unsigned int index = 0; index < theArray->size; ++index) {
        *o << "(get-value ( (select " << (**it).name << " (_ bv" << index << " "
           << theArray->getDomain() << ") ) ) )\n";
      }
    }
  }
}

void ExprSMTLIBPrinter::scan(const ref<Expr> &e) {
  assert(!(e.isNull()) && "found NULL expression");

  if (isa<ConstantExpr>(e))
    return; // we don't need to scan simple constants

  if (seenExprs.insert(e).second) {
    // We've not seen this expression before

    if (const ReadExpr *re = dyn_cast<ReadExpr>(e)) {

      if (usedArrays.insert(re->updates.root).second) {
        // Array was not recorded before

        // check if the array is constant
        if (re->updates.root->isConstantArray())
          haveConstantArray = true;

        // scan the update list
        scanUpdates(re->updates.head);
      }
    }

    // recurse into the children
    Expr *ep = e.get();
    for (unsigned int i = 0; i < ep->getNumKids(); i++)
      scan(ep->getKid(i));
  } else {
    // Add the expression to the binding map. The semantics of std::map::insert
    // are such that it will not be inserted twice.
    bindings.insert(std::make_pair(e, bindings.size()+1));
  }
}

void ExprSMTLIBPrinter::scanBindingExprDeps() {
  if (!bindings.size())
    return;

  // Mutual dependency storage
  typedef std::map<const ref<Expr>, std::set<ref<Expr> > > ExprDepMap;

  // A map from binding Expr (need abbreviating) "e" to the set of binding Expr
  // that are sub expressions of "e" (i.e. "e" uses these sub expressions).
  // usesSubExprMap[a].count(b) == 1  means that binding Expr "a" uses
  // sub Expr "b" (also a binding Expr).
  // One can think of this map representing the "depends on" edges
  // in a "dependency graph" where nodes are binding Expr roots
  ExprDepMap usesSubExprMap;

  // A map from Binding Expr "e" to the set of binding Expr that use "e"
  // subExprOfMap[a].count(b) == 1 means that binding Expr "a" is a sub Expr
  // of binding Expr "b".
  // One can think of this map as representing the edges in the previously
  // mentioned "dependency graph" except the direction of the edges
  // have been reversed
  ExprDepMap subExprOfMap;

  // Working queue holding expressions with no dependencies
  std::vector<ref<Expr> > nonDepBindings;

  // Iterate over bindings and collect dependencies
  for (BindingMap::const_iterator it = bindings.begin();
       it != bindings.end(); ++it) {
    std::stack<ref<Expr> > childQueue;
    childQueue.push(it->first);
    // Non-recursive expression parsing
    while (childQueue.size()) {
      Expr *ep = childQueue.top().get();
      childQueue.pop();
      for (unsigned i = 0; i < ep->getNumKids(); ++i) {
        ref<Expr> e = ep->getKid(i);
        if (isa<ConstantExpr>(e))
          continue;
        // Are there any dependencies in the bindings?
        if (bindings.count(e)) {
          usesSubExprMap[it->first].insert(e);
          subExprOfMap[e].insert(it->first);
        } else {
          childQueue.push(e);
        }
      }
    }
    // Store expressions with zero deps
    if (!usesSubExprMap.count(it->first))
      nonDepBindings.push_back(it->first);
  }
  assert(nonDepBindings.size() && "there must be expr bindings with no deps");

  // Unroll the dependency tree starting with zero-dep expressions
  // and redefine bindings
  unsigned counter = 1;
  // nonDepBindings always holds expressions with no dependencies
  while (nonDepBindings.size()) {
    BindingMap levelExprs;
    std::vector<ref<Expr> > tmp(nonDepBindings);
    nonDepBindings.clear();
    for (std::vector<ref<Expr> >::const_iterator nonDepExprIt = tmp.begin();
         nonDepExprIt != tmp.end(); ++nonDepExprIt) {
      // Save to the level expression bindings
      levelExprs.insert(std::make_pair(*nonDepExprIt, counter++));
      // Who is dependent on me?
      ExprDepMap::iterator depsIt = subExprOfMap.find(*nonDepExprIt);
      if (depsIt != subExprOfMap.end()) {
        for (std::set<ref<Expr> >::iterator exprIt = depsIt->second.begin();
             exprIt != depsIt->second.end(); ) {
          // Erase dependency
          ExprDepMap::iterator subExprIt = usesSubExprMap.find(*exprIt);
          assert(subExprIt != usesSubExprMap.end());
          assert(subExprIt->second.count(*nonDepExprIt));
          subExprIt->second.erase(*nonDepExprIt);
          // If the expression *exprIt does not have any
          // dependencies anymore, add it to the queue
          if (!subExprIt->second.size()) {
            nonDepBindings.push_back(*exprIt);
            depsIt->second.erase(exprIt++);
          } else {
            ++exprIt;
          }
        }
      }
    }
    // Store level bindings
    orderedBindings.push_back(levelExprs);
  }
}

void ExprSMTLIBPrinter::scanUpdates(const UpdateNode *un) {
  while (un != NULL) {
    scan(un->index);
    scan(un->value);
    un = un->next;
  }
}

void ExprSMTLIBPrinter::printExit() { *o << "(exit)\n"; }

bool ExprSMTLIBPrinter::setLogic(SMTLIBv2Logic l) {
  if (l > QF_AUFBV)
    return false;

  logicToUse = l;
  return true;
}

void ExprSMTLIBPrinter::printSeperator() {
  if (humanReadable)
    p->breakLineI();
  else
    p->write(" ");
}

void ExprSMTLIBPrinter::printNotice() {
  *o << "; This file conforms to SMTLIBv2 and was generated by KLEE\n";
}

void ExprSMTLIBPrinter::setHumanReadable(bool hr) { humanReadable = hr; }

void ExprSMTLIBPrinter::printOptions() {
  // Print out SMTLIBv2 boolean options
  for (std::map<SMTLIBboolOptions, bool>::const_iterator i =
           smtlibBoolOptions.begin();
       i != smtlibBoolOptions.end(); i++) {
    *o << "(set-option :" << getSMTLIBOptionString(i->first) << " "
       << ((i->second) ? "true" : "false") << ")\n";
  }
}

void ExprSMTLIBPrinter::printAssert(const ref<Expr> &e) {
  p->pushIndent();
  *p << "(assert";
  p->pushIndent();
  printSeperator();

  if (abbrMode == ABBR_LET && orderedBindings.size() != 0) {
    // Only print let expression if we have bindings to use.
    *p << "(let";
    p->pushIndent();
    printSeperator();
    *p << "(";
    p->pushIndent();

    // Clear original bindings, we'll be using orderedBindings
    // to print nested let expressions
    bindings.clear();

    // Print each binding on its level
    for (unsigned i = 0; i < orderedBindings.size(); ++i) {
      BindingMap levelBindings = orderedBindings[i];
      for (BindingMap::const_iterator j = levelBindings.begin();
           j != levelBindings.end(); ++j) {
        printSeperator();
        *p << "(?B" << j->second;
        p->pushIndent();
        printSeperator();

        // We can abbreviate SORT_BOOL or SORT_BITVECTOR in let expressions
        printExpression(j->first, getSort(j->first));

        p->popIndent();
        printSeperator();
        *p << ")";
      }
      p->popIndent();
      printSeperator();
      *p << ")";
      printSeperator();

      // Add nested let expressions (if any)
      if (i < orderedBindings.size()-1) {
        *p << "(let";
        p->pushIndent();
        printSeperator();
        *p << "(";
        p->pushIndent();
      }
      // Insert current level bindings so that they can be used
      // in the next level during expression printing
      bindings.insert(levelBindings.begin(), levelBindings.end());
    }

    printExpression(e, SORT_BOOL);

    for (unsigned i = 0; i < orderedBindings.size(); ++i) {
      p->popIndent();
      printSeperator();
      *p << ")";
    }
  } else {
    printExpression(e, SORT_BOOL);
  }

  p->popIndent();
  printSeperator();
  *p << ")";
  p->popIndent();
  p->breakLineI();
}

ExprSMTLIBPrinter::SMTLIB_SORT ExprSMTLIBPrinter::getSort(const ref<Expr> &e) {
  switch (e->getKind()) {
  case Expr::NotOptimized:
    return getSort(e->getKid(0));

  // The relational operators are bools.
  case Expr::Eq:
  case Expr::Ne:
  case Expr::Slt:
  case Expr::Sle:
  case Expr::Sgt:
  case Expr::Sge:
  case Expr::Ult:
  case Expr::Ule:
  case Expr::Ugt:
  case Expr::Uge:
    return SORT_BOOL;

  // These may be bitvectors or bools depending on their width (see
  // printConstant and printLogicalOrBitVectorExpr).
  case Expr::Constant:
  case Expr::And:
  case Expr::Not:
  case Expr::Or:
  case Expr::Xor:
    return e->getWidth() == Expr::Bool ? SORT_BOOL : SORT_BITVECTOR;

  // Everything else is a bitvector.
  default:
    return SORT_BITVECTOR;
  }
}

void ExprSMTLIBPrinter::printCastToSort(const ref<Expr> &e,
                                        ExprSMTLIBPrinter::SMTLIB_SORT sort) {
  switch (sort) {
  case SORT_BITVECTOR:
    if (humanReadable) {
      p->breakLineI();
      *p << ";Performing implicit bool to bitvector cast";
      p->breakLine();
    }
    // We assume the e is a bool that we need to cast to a bitvector sort.
    *p << "(ite";
    p->pushIndent();
    printSeperator();
    printExpression(e, SORT_BOOL);
    printSeperator();
    *p << "(_ bv1 1)";
    printSeperator(); // printing the "true" bitvector
    *p << "(_ bv0 1)";
    p->popIndent();
    printSeperator(); // printing the "false" bitvector
    *p << ")";
    break;
  case SORT_BOOL: {
    /* We make the assumption (might be wrong) that any bitvector whose unsigned
     * decimal value is is zero is interpreted as "false", otherwise it is
     * true.
     *
     * This may not be the interpretation we actually want!
     */
    Expr::Width bitWidth = e->getWidth();
    if (humanReadable) {
      p->breakLineI();
      *p << ";Performing implicit bitvector to bool cast";
      p->breakLine();
    }
    *p << "(bvugt";
    p->pushIndent();
    printSeperator();
    // We assume is e is a bitvector
    printExpression(e, SORT_BITVECTOR);
    printSeperator();
    *p << "(_ bv0 " << bitWidth << ")";
    p->popIndent();
    printSeperator(); // Zero bitvector of required width
    *p << ")";

    if (bitWidth != Expr::Bool)
      llvm::errs()
          << "ExprSMTLIBPrinter : Warning. Casting a bitvector (length "
          << bitWidth << ") to bool!\n";

  } break;
  default:
    llvm_unreachable("Unsupported cast");
  }
}

void ExprSMTLIBPrinter::printSelectExpr(const ref<SelectExpr> &e,
                                        ExprSMTLIBPrinter::SMTLIB_SORT s) {
  // This is the if-then-else expression

  *p << "(" << getSMTLIBKeyword(e) << " ";
  p->pushIndent(); // add indent for recursive call

  // The condition
  printSeperator();
  printExpression(e->getKid(0), SORT_BOOL);

  /* This operator is special in that the remaining children
   * can be of any sort.
   */

  // if true
  printSeperator();
  printExpression(e->getKid(1), s);

  // if false
  printSeperator();
  printExpression(e->getKid(2), s);

  p->popIndent(); // pop indent added for recursive call
  printSeperator();
  *p << ")";
}

void ExprSMTLIBPrinter::printSortArgsExpr(const ref<Expr> &e,
                                          ExprSMTLIBPrinter::SMTLIB_SORT s) {
  *p << "(" << getSMTLIBKeyword(e) << " ";
  p->pushIndent(); // add indent for recursive call

  // loop over children and recurse into each expecting they are of sort "s"
  for (unsigned int i = 0; i < e->getNumKids(); i++) {
    printSeperator();
    printExpression(e->getKid(i), s);
  }

  p->popIndent(); // pop indent added for recursive call
  printSeperator();
  *p << ")";
}

void ExprSMTLIBPrinter::printLogicalOrBitVectorExpr(
    const ref<Expr> &e, ExprSMTLIBPrinter::SMTLIB_SORT s) {
  /* For these operators it is the case that the expected sort is the same as
   * the sorts
   * of the arguments.
   */

  *p << "(";
  switch (e->getKind()) {
  case Expr::And:
    *p << ((s == SORT_BITVECTOR) ? "bvand" : "and");
    break;
  case Expr::Not:
    *p << ((s == SORT_BITVECTOR) ? "bvnot" : "not");
    break;
  case Expr::Or:
    *p << ((s == SORT_BITVECTOR) ? "bvor" : "or");
    break;

  case Expr::Xor:
    *p << ((s == SORT_BITVECTOR) ? "bvxor" : "xor");
    break;
  default:
    llvm_unreachable("Conversion from Expr to SMTLIB keyword failed");
  }
  *p << " ";

  p->pushIndent(); // add indent for recursive call

  // loop over children and recurse into each expecting they are of sort "s"
  for (unsigned int i = 0; i < e->getNumKids(); i++) {
    printSeperator();
    printExpression(e->getKid(i), s);
  }

  p->popIndent(); // pop indent added for recursive call
  printSeperator();
  *p << ")";
}

bool ExprSMTLIBPrinter::setSMTLIBboolOption(SMTLIBboolOptions option,
                                            SMTLIBboolValues value) {
  std::pair<std::map<SMTLIBboolOptions, bool>::iterator, bool> thePair;
  bool theValue = (value == OPTION_TRUE) ? true : false;

  switch (option) {
  case PRINT_SUCCESS:
  case PRODUCE_MODELS:
  case INTERACTIVE_MODE:
    thePair = smtlibBoolOptions.insert(
        std::pair<SMTLIBboolOptions, bool>(option, theValue));

    if (value == OPTION_DEFAULT) {
      // we should unset (by removing from map) this option so the solver uses
      // its default
      smtlibBoolOptions.erase(thePair.first);
      return true;
    }

    if (!thePair.second) {
      // option was already present so modify instead.
      thePair.first->second = value;
    }
    return true;
  default:
    return false;
  }
}

void
ExprSMTLIBPrinter::setArrayValuesToGet(const std::vector<const Array *> &a) {
  arraysToCallGetValueOn = &a;

  // This option must be set in order to use the SMTLIBv2 command (get-value ()
  // )
  if (!a.empty())
    setSMTLIBboolOption(PRODUCE_MODELS, OPTION_TRUE);

  /* There is a risk that users will ask about array values that aren't
   * even in the query. We should add them to the usedArrays list and hope
   * that the solver knows what to do when we ask for the values of arrays
   * that don't feature in our query!
   */
  for (std::vector<const Array *>::const_iterator i = a.begin(); i != a.end();
       ++i) {
    usedArrays.insert(*i);
  }
}

const char *ExprSMTLIBPrinter::getSMTLIBOptionString(
    ExprSMTLIBPrinter::SMTLIBboolOptions option) {
  switch (option) {
  case PRINT_SUCCESS:
    return "print-success";
  case PRODUCE_MODELS:
    return "produce-models";
  case INTERACTIVE_MODE:
    return "interactive-mode";
  default:
    return "unknown-option";
  }
}
}
