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
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/Casting.h"
#include "klee/Support/OptionCategories.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

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

namespace klee {
cl::opt<unsigned long long> MaxCyclesBeforeStuck(
    "max-cycles-before-stuck",
    cl::desc("Set target after after state visiting some basic block this "
             "amount of times (default=1)."),
    cl::init(1), cl::cat(TerminationCat));
}

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

void ExecutionStack::pushFrame(KInstIterator caller, KFunction *kf) {
  valueStack_.emplace_back(StackFrame(kf));
  if (std::find(callStack_.begin(), callStack_.end(),
                CallStackFrame(caller, kf)) == callStack_.end()) {
    uniqueFrames_.emplace_back(CallStackFrame(caller, kf));
  }
  callStack_.emplace_back(CallStackFrame(caller, kf));
  infoStack_.emplace_back(InfoStackFrame(kf));
  auto kfLevel = multilevel[kf].second;
  multilevel.replace({kf, kfLevel + 1});
  ++stackBalance;
  assert(valueStack_.size() == callStack_.size());
  assert(valueStack_.size() == infoStack_.size());
  stackSize += kf->getNumRegisters();
}

void ExecutionStack::popFrame() {
  assert(callStack_.size() > 0);
  KInstIterator caller = callStack_.back().caller;
  KFunction *kf = callStack_.back().kf;
  valueStack_.pop_back();
  callStack_.pop_back();
  infoStack_.pop_back();
  auto it = std::find(callStack_.begin(), callStack_.end(),
                      CallStackFrame(caller, kf));
  if (it == callStack_.end()) {
    uniqueFrames_.pop_back();
  }
  auto kfLevel = multilevel[kf].second;
  multilevel.replace({kf, kfLevel - 1});
  --stackBalance;
  assert(valueStack_.size() == callStack_.size());
  assert(valueStack_.size() == infoStack_.size());
  stackSize -= kf->getNumRegisters();
}

bool CallStackFrame::equals(const CallStackFrame &other) const {
  return kf == other.kf && caller == other.caller;
}

StackFrame::StackFrame(KFunction *kf) : kf(kf), varargs(nullptr) {
  locals = new Cell[kf->getNumRegisters()];
}

StackFrame::StackFrame(const StackFrame &s)
    : kf(s.kf), allocas(s.allocas), varargs(s.varargs) {
  locals = new Cell[kf->getNumRegisters()];
  for (unsigned i = 0; i < kf->getNumRegisters(); i++)
    locals[i] = s.locals[i];
}

StackFrame::~StackFrame() { delete[] locals; }

InfoStackFrame::InfoStackFrame(KFunction *kf) : kf(kf) {}

/***/
ExecutionState::ExecutionState()
    : initPC(nullptr), pc(nullptr), prevPC(nullptr), incomingBBIndex(-1),
      depth(0), ptreeNode(nullptr), symbolics(), steppedInstructions(0),
      steppedMemoryInstructions(0), instsSinceCovNew(0),
      roundingMode(llvm::APFloat::rmNearestTiesToEven), coveredNew({}),
      coveredNewError(new box<bool>(false)), forkDisabled(false),
      prevHistory_(TargetsHistory::create()),
      history_(TargetsHistory::create()) {
  setID();
}

ExecutionState::ExecutionState(KFunction *kf)
    : initPC(kf->instructions), pc(initPC), prevPC(pc), incomingBBIndex(-1),
      depth(0), ptreeNode(nullptr), symbolics(), steppedInstructions(0),
      steppedMemoryInstructions(0), instsSinceCovNew(0),
      roundingMode(llvm::APFloat::rmNearestTiesToEven), coveredNew({}),
      coveredNewError(new box<bool>(false)), forkDisabled(false),
      prevHistory_(TargetsHistory::create()),
      history_(TargetsHistory::create()) {
  pushFrame(nullptr, kf);
  setID();
}

ExecutionState::ExecutionState(KFunction *kf, KBlock *kb)
    : initPC(kb->instructions), pc(initPC), prevPC(pc), incomingBBIndex(-1),
      depth(0), ptreeNode(nullptr), symbolics(), steppedInstructions(0),
      steppedMemoryInstructions(0), instsSinceCovNew(0),
      roundingMode(llvm::APFloat::rmNearestTiesToEven), coveredNew({}),
      coveredNewError(new box<bool>(false)), forkDisabled(false),
      prevHistory_(TargetsHistory::create()),
      history_(TargetsHistory::create()) {
  pushFrame(nullptr, kf);
  setID();
}

ExecutionState::~ExecutionState() {
  while (!stack.empty())
    popFrame();
}

ExecutionState::ExecutionState(const ExecutionState &state)
    : initPC(state.initPC), pc(state.pc), prevPC(state.prevPC),
      stack(state.stack), stackBalance(state.stackBalance),
      incomingBBIndex(state.incomingBBIndex), depth(state.depth),
      level(state.level), addressSpace(state.addressSpace),
      constraints(state.constraints), targetForest(state.targetForest),
      pathOS(state.pathOS), symPathOS(state.symPathOS),
      coveredLines(state.coveredLines), symbolics(state.symbolics),
      resolvedPointers(state.resolvedPointers),
      cexPreferences(state.cexPreferences), arrayNames(state.arrayNames),
      steppedInstructions(state.steppedInstructions),
      steppedMemoryInstructions(state.steppedMemoryInstructions),
      instsSinceCovNew(state.instsSinceCovNew),
      roundingMode(state.roundingMode),
      unwindingInformation(state.unwindingInformation
                               ? state.unwindingInformation->clone()
                               : nullptr),
      coveredNew(state.coveredNew), coveredNewError(state.coveredNewError),
      forkDisabled(state.forkDisabled), returnValue(state.returnValue),
      gepExprBases(state.gepExprBases), prevTargets_(state.prevTargets_),
      targets_(state.targets_), prevHistory_(state.prevHistory_),
      history_(state.history_), isTargeted_(state.isTargeted_) {
  constraints.fork();
  queryMetaData.id = state.id;
}

ExecutionState *ExecutionState::branch() {
  depth++;

  auto *falseState = new ExecutionState(*this);
  falseState->setID();
  falseState->coveredLines.clear();

  return falseState;
}

bool ExecutionState::inSymbolics(const MemoryObject *mo) const {
  for (const auto &symbolic : symbolics) {
    if (mo->id == symbolic.memoryObject->id) {
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
  newState->initPC = kf->blockMap[&*kf->function()->begin()]->instructions;
  newState->pc = newState->initPC;
  newState->prevPC = newState->pc;
  return newState;
}

ExecutionState *ExecutionState::withKInstruction(KInstruction *ki) const {
  assert(stack.size() == 0);
  ExecutionState *newState = new ExecutionState(*this);
  newState->setID();
  newState->pushFrame(nullptr, ki->parent->parent);
  newState->stackBalance = 0;
  newState->initPC = ki->parent->instructions;
  while (newState->initPC != ki) {
    ++newState->initPC;
  }
  newState->pc = newState->initPC;
  newState->prevPC = newState->pc;
  return newState;
}

ExecutionState *ExecutionState::withKFunction(KFunction *kf) {
  return withStackFrame(nullptr, kf);
}

ExecutionState *ExecutionState::copy() const {
  ExecutionState *newState = new ExecutionState(*this);
  newState->setID();
  return newState;
}

void ExecutionState::pushFrame(KInstIterator caller, KFunction *kf) {
  stack.pushFrame(caller, kf);
}

void ExecutionState::popFrame() {
  const StackFrame &sf = stack.valueStack().back();
  for (const auto id : sf.allocas) {
    const MemoryObject *memoryObject = addressSpace.findObject(id).first;
    assert(memoryObject);
    removePointerResolutions(memoryObject);
    addressSpace.unbindObject(memoryObject);
  }
  stack.popFrame();
}

void ExecutionState::addSymbolic(const MemoryObject *mo, const Array *array,
                                 KType *type) {
  symbolics.push_back({mo, array, type});
}

ref<const MemoryObject>
ExecutionState::findMemoryObject(const Array *array) const {
  for (const auto &symbolic : symbolics) {
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
  for (auto resolution = begin(resolvedPointers);
       resolution != end(resolvedPointers);) {
    resolution->second.erase(mo->id);
    if (resolution->second.size() == 0) {
      resolution = resolvedPointers.erase(resolution);
    } else {
      ++resolution;
    }
  }

  for (auto resolution = begin(resolvedSubobjects);
       resolution != end(resolvedSubobjects);) {
    resolution->second.erase(mo->id);
    if (resolution->second.size() == 0) {
      resolution = resolvedSubobjects.erase(resolution);
    } else {
      ++resolution;
    }
  }
}

void ExecutionState::removePointerResolutions(ref<Expr> address,
                                              unsigned size) {
  if (!isa<ConstantExpr>(address)) {
    resolvedPointers[address].clear();
    resolvedSubobjects[MemorySubobject(address, size)].clear();
  }
}

// base address mo and ignore non pure reads in setinitializationgraph
void ExecutionState::addPointerResolution(ref<Expr> address,
                                          const MemoryObject *mo,
                                          unsigned size) {
  if (!isa<ConstantExpr>(address)) {
    resolvedPointers[address].insert(mo->id);
    resolvedSubobjects[MemorySubobject(address, size)].insert(mo->id);
  }
}

void ExecutionState::addUniquePointerResolution(ref<Expr> address,
                                                const MemoryObject *mo,
                                                unsigned size) {
  if (!isa<ConstantExpr>(address)) {
    removePointerResolutions(address, size);
    resolvedPointers[address].insert(mo->id);
    resolvedSubobjects[MemorySubobject(address, size)].insert(mo->id);
  }
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
  const KInstruction *target = prevPC;
  for (unsigned i = 0; i < stack.size(); ++i) {
    unsigned ri = stack.size() - 1 - i;
    const CallStackFrame &csf = stack.callStack().at(ri);
    const StackFrame &sf = stack.valueStack().at(ri);

    Function *f = csf.kf->function();
    out << "\t#" << i;
    auto assemblyLine = target->getKModule()->getAsmLine(target->inst());
    if (assemblyLine.has_value()) {
      std::stringstream AsmStream;
      AsmStream << std::setw(8) << std::setfill('0') << assemblyLine.value();
      out << AsmStream.str();
    }
    out << " in " << f->getName().str() << "(";
    // Yawn, we could go up and print varargs if we wanted to.
    unsigned index = 0;
    for (Function::arg_iterator ai = f->arg_begin(), ae = f->arg_end();
         ai != ae; ++ai) {
      if (ai != f->arg_begin())
        out << ", ";

      if (ai->hasName())
        out << ai->getName().str() << "=";

      ref<Expr> value = sf.locals[csf.kf->getArgRegister(index++)].value;
      if (isa_and_nonnull<ConstantExpr>(value)) {
        out << value;
      } else {
        out << "symbolic";
      }
    }
    out << ")";
    std::string filepath = target->getSourceFilepath();
    if (!filepath.empty())
      out << " at " << filepath << ":" << target->getLine();
    out << "\n";
    target = csf.caller;
  }
}

void ExecutionState::addConstraint(ref<Expr> e, const Assignment &delta) {
  constraints.addConstraint(e, delta);
}

void ExecutionState::addCexPreference(const ref<Expr> &cond) {
  cexPreferences = cexPreferences.insert(cond);
}

BasicBlock *ExecutionState::getInitPCBlock() const {
  return initPC->inst()->getParent();
}

BasicBlock *ExecutionState::getPrevPCBlock() const {
  return prevPC->inst()->getParent();
}

BasicBlock *ExecutionState::getPCBlock() const {
  return pc->inst()->getParent();
}

void ExecutionState::increaseLevel() {
  llvm::BasicBlock *srcbb = getPrevPCBlock();
  llvm::BasicBlock *dstbb = getPCBlock();
  KFunction *kf = prevPC->parent->parent;
  KModule *kmodule = kf->parent;

  if (prevPC->inst()->isTerminator() &&
      kmodule->inMainModule(*kf->function())) {
    auto srcLevel = stack.infoStack().back().multilevel[srcbb].second;
    stack.infoStack().back().multilevel.replace({srcbb, srcLevel + 1});
    level.insert(prevPC->parent);
  }
}

bool ExecutionState::isGEPExpr(ref<Expr> expr) const {
  return UseGEPOptimization && gepExprBases.find(expr) != gepExprBases.end();
}

bool ExecutionState::visited(KBlock *block) const {
  return level.count(block) != 0;
}

bool ExecutionState::reachedTarget(ref<ReachBlockTarget> target) const {
  if (constraints.path().KBlockSize() == 0) {
    return false;
  }
  if (target->isAtEnd()) {
    return prevPC == target->getBlock()->getLastInstruction();
  } else {
    return pc == target->getBlock()->getFirstInstruction();
  }
}

Query ExecutionState::toQuery(ref<Expr> head) const {
  return Query(constraints.cs(), head, id);
}

Query ExecutionState::toQuery() const { return toQuery(Expr::createFalse()); }
