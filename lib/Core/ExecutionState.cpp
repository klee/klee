//===-- ExecutionState.cpp ------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ExecutionState.h"

#include "Memory.h"

#include "klee/Expr/ArrayExprVisitor.h"
#include "klee/Expr/Expr.h"
#include "klee/Module/Cell.h"
#include "klee/Module/InstructionInfoTable.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/Casting.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdarg.h>
#include <string>

using namespace llvm;
using namespace klee;

namespace {
cl::opt<bool> UseGEPOptimization(
    "use-gep-opt", cl::init(true),
    cl::desc("Lazily initialize whole objects referenced by gep expressions "
             "instead of only the referenced parts (default=true)"),
    cl::cat(ExecCat));
} // namespace

/***/

std::uint32_t ExecutionState::nextID = 1;

/***/

StackFrame::StackFrame(KInstIterator _caller, KFunction *_kf)
    : caller(_caller), kf(_kf), callPathNode(0), minDistToUncoveredOnReturn(0),
      varargs(0) {
  locals = new Cell[kf->numRegisters];
}

StackFrame::StackFrame(const StackFrame &s)
    : caller(s.caller), kf(s.kf), callPathNode(s.callPathNode),
      allocas(s.allocas),
      minDistToUncoveredOnReturn(s.minDistToUncoveredOnReturn),
      varargs(s.varargs) {
  locals = new Cell[s.kf->numRegisters];
  for (unsigned i = 0; i < s.kf->numRegisters; i++)
    locals[i] = s.locals[i];
}

StackFrame::~StackFrame() { delete[] locals; }

/***/
ExecutionState::ExecutionState()
    : initPC(nullptr), pc(nullptr), prevPC(nullptr), incomingBBIndex(-1),
      depth(0), ptreeNode(nullptr), steppedInstructions(0),
      steppedMemoryInstructions(0), instsSinceCovNew(0),
      roundingMode(llvm::APFloat::rmNearestTiesToEven), coveredNew(false),
      forkDisabled(false) {
  setID();
}

ExecutionState::ExecutionState(KFunction *kf)
    : initPC(kf->instructions), pc(initPC), prevPC(pc),
      roundingMode(llvm::APFloat::rmNearestTiesToEven) {
  pushFrame(nullptr, kf);
  setID();
}

ExecutionState::ExecutionState(KFunction *kf, KBlock *kb)
    : initPC(kb->instructions), pc(initPC), prevPC(pc), incomingBBIndex(-1),
      depth(0), ptreeNode(nullptr), steppedInstructions(0),
      steppedMemoryInstructions(0), instsSinceCovNew(0),
      roundingMode(llvm::APFloat::rmNearestTiesToEven), coveredNew(false),
      forkDisabled(false) {
  pushFrame(nullptr, kf);
  setID();
}

ExecutionState::~ExecutionState() {
  while (!stack.empty())
    popFrame();
}

ExecutionState::ExecutionState(const ExecutionState &state)
    : initPC(state.initPC), pc(state.pc), prevPC(state.prevPC),
      stack(state.stack), incomingBBIndex(state.incomingBBIndex),
      depth(state.depth), multilevel(state.multilevel), level(state.level),
      addressSpace(state.addressSpace), constraints(state.constraints),
      targetForest(state.targetForest), pathOS(state.pathOS),
      symPathOS(state.symPathOS), coveredLines(state.coveredLines),
      symbolics(state.symbolics), symbolicSizes(state.symbolicSizes),
      resolvedPointers(state.resolvedPointers),
      cexPreferences(state.cexPreferences), arrayNames(state.arrayNames),
      steppedInstructions(state.steppedInstructions),
      steppedMemoryInstructions(state.steppedMemoryInstructions),
      instsSinceCovNew(state.instsSinceCovNew),
      roundingMode(state.roundingMode),
      unwindingInformation(state.unwindingInformation
                               ? state.unwindingInformation->clone()
                               : nullptr),
      coveredNew(state.coveredNew), forkDisabled(state.forkDisabled),
      gepExprBases(state.gepExprBases), gepExprOffsets(state.gepExprOffsets) {
}

ExecutionState *ExecutionState::branch() {
  depth++;

  auto *falseState = new ExecutionState(*this);
  falseState->setID();
  falseState->coveredNew = false;
  falseState->coveredLines.clear();

  return falseState;
}

bool ExecutionState::inSymbolics(const MemoryObject *mo) const {
  for (auto i : symbolics) {
    if (mo->id == i.memoryObject->id) {
      return true;
    }
  }
  return false;
}

ExecutionState *ExecutionState::withStackFrame(KInstIterator caller,
                                               KFunction *kf) {
  ExecutionState *newState = new ExecutionState(*this);
  newState->setID();
  newState->pushFrame(caller, kf);
  newState->initPC = kf->blockMap[&*kf->function->begin()]->instructions;
  newState->pc = newState->initPC;
  newState->prevPC = newState->pc;
  return newState;
}

ExecutionState *ExecutionState::withKFunction(KFunction *kf) {
  return withStackFrame(nullptr, kf);
}

ExecutionState *ExecutionState::withKBlock(KBlock *kb) {
  ExecutionState *newState = new ExecutionState(*this);
  newState->setID();
  newState->initPC = kb->instructions;
  newState->pc = newState->initPC;
  newState->prevPC = newState->pc;
  return newState;
}

ExecutionState *ExecutionState::copy() {
  ExecutionState *newState = new ExecutionState(*this);
  newState->setID();
  return newState;
}

void ExecutionState::pushFrame(KInstIterator caller, KFunction *kf) {
  stack.emplace_back(StackFrame(caller, kf));
}

void ExecutionState::popFrame() {
  const StackFrame &sf = stack.back();
  for (const auto id : sf.allocas) {
    const MemoryObject *memoryObject = addressSpace.findObject(id).first;
    assert(memoryObject);
    removePointerResolutions(memoryObject);
    addressSpace.unbindObject(memoryObject);
  }
  stack.pop_back();
}

void ExecutionState::addSymbolic(const MemoryObject *mo, const Array *array,
                                 KType *type) {
  symbolics.emplace_back(ref<const MemoryObject>(mo), array, type);
}

ref<const MemoryObject>
ExecutionState::findMemoryObject(const Array *array) const {
  for (unsigned i = 0; i != symbolics.size(); ++i) {
    const auto &symbolic = symbolics[i];
    if (array == symbolic.array) {
      return symbolic.memoryObject;
    }
  }
  return nullptr;
}

bool ExecutionState::getBase(
    ref<Expr> expr,
    std::pair<ref<const MemoryObject>, ref<Expr>> &resolution) const {
  switch (expr->getKind()) {
  case Expr::Read: {
    ref<ReadExpr> base = dyn_cast<ReadExpr>(expr);
    auto parent = findMemoryObject(base->updates.root);
    if (!parent) {
      return false;
    }
    resolution = std::make_pair(parent, base->index);
    return true;
  }
  case Expr::Concat: {
    ref<ReadExpr> base =
        ArrayExprHelper::hasOrderedReads(*dyn_cast<ConcatExpr>(expr));
    if (!base) {
      return false;
    }
    auto parent = findMemoryObject(base->updates.root);
    if (!parent) {
      return false;
    }
    resolution = std::make_pair(parent, base->index);
    return true;
  }
  default: {
    if (isGEPExpr(expr)) {
      ref<Expr> gepBase = gepExprBases.at(expr).first;
      std::pair<ref<const MemoryObject>, ref<Expr>> gepResolved;
      if (expr != gepBase && getBase(gepBase, gepResolved)) {
        auto parent = gepResolved.first;
        auto index = gepResolved.second;
        resolution = std::make_pair(parent, index);
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  }
}

void ExecutionState::removePointerResolutions(const MemoryObject *mo) {
  for (auto i = resolvedPointers.begin(), last = resolvedPointers.end();
       i != last;) {
    if (i->second.first == mo->id) {
      i = resolvedPointers.erase(i);
    } else {
      ++i;
    }
  }
}

// base address mo and ignore non pure reads in setinitializationgraph
void ExecutionState::addPointerResolution(ref<Expr> address, ref<Expr> base,
                                          const MemoryObject *mo) {
  if (!isa<ConstantExpr>(address)) {
    resolvedPointers[address] =
        std::make_pair(mo->id, mo->getOffsetExpr(address));
  }
  if (base != address && !isa<ConstantExpr>(base)) {
    resolvedPointers[base] = std::make_pair(mo->id, mo->getOffsetExpr(base));
  }
}

bool ExecutionState::resolveOnSymbolics(const ref<ConstantExpr> &addr,
                                        IDType &result) const {
  uint64_t address = addr->getZExtValue();

  for (const auto &res : symbolics) {
    const auto &mo = res.memoryObject;
    // Check if the provided address is between start and end of the object
    // [mo->address, mo->address + mo->size) or the object is a 0-sized object.
    ref<ConstantExpr> size = cast<ConstantExpr>(
        constraints.getConcretization().evaluate(mo->getSizeExpr()));
    if ((size->getZExtValue() == 0 && address == mo->address) ||
        (address - mo->address < size->getZExtValue())) {
      result = mo->id;
      return true;
    }
  }

  return false;
}

/**/

llvm::raw_ostream &klee::operator<<(llvm::raw_ostream &os,
                                    const MemoryMap &mm) {
  os << "{";
  MemoryMap::iterator it = mm.begin();
  MemoryMap::iterator ie = mm.end();
  if (it != ie) {
    os << "MO" << it->first->id << ":" << it->second.get();
    for (++it; it != ie; ++it)
      os << ", MO" << it->first->id << ":" << it->second.get();
  }
  os << "}";
  return os;
}

void ExecutionState::dumpStack(llvm::raw_ostream &out) const {
  unsigned idx = 0;
  const KInstruction *target = prevPC;
  for (ExecutionState::stack_ty::const_reverse_iterator it = stack.rbegin(),
                                                        ie = stack.rend();
       it != ie; ++it) {
    const StackFrame &sf = *it;
    Function *f = sf.kf->function;
    const InstructionInfo &ii = *target->info;
    out << "\t#" << idx++;
    if (ii.assemblyLine.hasValue()) {
      std::stringstream AssStream;
      AssStream << std::setw(8) << std::setfill('0')
                << ii.assemblyLine.getValue();
      out << AssStream.str();
    }
    out << " in " << f->getName().str() << " (";
    // Yawn, we could go up and print varargs if we wanted to.
    unsigned index = 0;
    for (Function::arg_iterator ai = f->arg_begin(), ae = f->arg_end();
         ai != ae; ++ai) {
      if (ai != f->arg_begin())
        out << ", ";

      out << ai->getName().str();
      // XXX should go through function
      ref<Expr> value = sf.locals[sf.kf->getArgRegister(index++)].value;
      if (isa_and_nonnull<ConstantExpr>(value))
        out << "=" << value;
    }
    out << ")";
    if (ii.file != "")
      out << " at " << ii.file << ":" << ii.line;
    out << "\n";
    target = sf.caller;
  }
}

void ExecutionState::addConstraint(ref<Expr> e) {
  ConstraintManager cm(constraints);
  cm.addConstraint(e);
}

void ExecutionState::addConstraint(ref<Expr> e, const Assignment &c) {
  ConstraintManager cm(constraints);
  cm.addConstraint(e, c);
}

void ExecutionState::addCexPreference(const ref<Expr> &cond) {
  cexPreferences = cexPreferences.insert(cond);
}

BasicBlock *ExecutionState::getInitPCBlock() const {
  return initPC->inst->getParent();
}

BasicBlock *ExecutionState::getPrevPCBlock() const {
  return prevPC->inst->getParent();
}

BasicBlock *ExecutionState::getPCBlock() const { return pc->inst->getParent(); }

void ExecutionState::increaseLevel() {
  llvm::BasicBlock *srcbb = getPrevPCBlock();
  llvm::BasicBlock *dstbb = getPCBlock();
  KFunction *kf = prevPC->parent->parent;
  KModule *kmodule = kf->parent;

  if (prevPC->inst->isTerminator() && kmodule->inMainModule(kf->function)) {
    multilevel.insert(srcbb);
    level.insert(srcbb);
  }
  transitionLevel.insert(std::make_pair(srcbb, dstbb));
}

bool ExecutionState::isTransfered() { return getPrevPCBlock() != getPCBlock(); }

bool ExecutionState::isGEPExpr(ref<Expr> expr) const {
  return UseGEPOptimization && gepExprBases.find(expr) != gepExprBases.end();
}

bool ExecutionState::visited(KBlock *block) const {
  return level.find(block->basicBlock) != level.end();
}
