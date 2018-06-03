//===-- SpecialFunctionHandler.cpp ----------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Memory.h"
#include "SpecialFunctionHandler.h"
#include "TimingSolver.h"

#include "klee/ExecutionState.h"

#include "klee/Internal/Module/KInstruction.h"
#include "klee/Internal/Module/KModule.h"
#include "klee/Internal/Support/Debug.h"
#include "klee/Internal/Support/ErrorHandling.h"

#include "Executor.h"
#include "MemoryManager.h"

#include "klee/CommandLine.h"

#include "llvm/IR/Module.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"

#include "llvm/IR/Constants.h"
#include <errno.h>

using namespace llvm;
using namespace klee;

namespace {
  cl::opt<bool>
  ReadablePosix("readable-posix-inputs",
            cl::init(false),
            cl::desc("Prefer creation of POSIX inputs (command-line arguments, files, etc.) with human readable bytes. "
                     "Note: option is expensive when creating lots of tests (default=false)"));

  cl::opt<bool>
  SilentKleeAssume("silent-klee-assume",
                   cl::init(false),
                   cl::desc("Silently terminate paths with an infeasible "
                            "condition given to klee_assume() rather than "
                            "emitting an error (default=false)"));
}


/// \todo Almost all of the demands in this file should be replaced
/// with terminateState calls.

///



// FIXME: We are more or less committed to requiring an intrinsic
// library these days. We can move some of this stuff there,
// especially things like realloc which have complicated semantics
// w.r.t. forking. Among other things this makes delayed query
// dispatch easier to implement.
static SpecialFunctionHandler::HandlerInfo handlerInfo[] = {
#define add(name, handler, ret) { name, \
                                  &SpecialFunctionHandler::handler, \
                                  false, ret, false }
#define addDNR(name, handler) { name, \
                                &SpecialFunctionHandler::handler, \
                                true, false, false }
  addDNR("__assert_rtn", handleAssertFail),
  addDNR("__assert_fail", handleAssertFail),
  addDNR("_assert", handleAssert),
  addDNR("abort", handleAbort),
  addDNR("_exit", handleExit),
  { "exit", &SpecialFunctionHandler::handleExit, true, false, true },
  addDNR("klee_abort", handleAbort),
  addDNR("klee_silent_exit", handleSilentExit),  
  addDNR("klee_report_error", handleReportError),

  add("calloc", handleCalloc, true),
  add("free", handleFree, false),
  add("klee_assume", handleAssume, false),
  add("klee_check_memory_access", handleCheckMemoryAccess, false),
  add("klee_get_valuef", handleGetValue, true),
  add("klee_get_valued", handleGetValue, true),
  add("klee_get_valuel", handleGetValue, true),
  add("klee_get_valuell", handleGetValue, true),
  /*************************************/
  /* true is for returning a value ... */
  /*************************************/
  add("MyPrintOutput",               handleMyPrintOutput,               false),
  add("MyAtoi",                      handleMyAtoi,                      true),
  add("markString",                  handleMarkString,                    false),
  add("strcpy",                      handleStrcpy,                    false),
  add("strncpy",                     handleStrncpy,                   false),
  add("strchr",                      handleStrchr,                    true),
  add("strrchr",                     handleStrrchr,                   true),
  add("strcmp",                      handleStrcmp,                    true),
  add("myStrncmp",                   handleStrncmp,                   true),
  add("strlen",                      handleStrlen,                    true),
  add("BREAKPOINT",                  handleBREAKPOINT,                false),
  add("klee_get_value_i32", handleGetValue, true),
  add("klee_get_value_i64", handleGetValue, true),
  add("klee_define_fixed_object", handleDefineFixedObject, false),
  add("klee_get_obj_size", handleGetObjSize, true),
  add("klee_get_errno", handleGetErrno, true),
  add("klee_is_symbolic", handleIsSymbolic, true),
  add("klee_make_symbolic", handleMakeSymbolic, false),
  add("klee_mark_global", handleMarkGlobal, false),
  add("klee_merge", handleMerge, false),
  add("klee_prefer_cex", handlePreferCex, false),
  add("klee_posix_prefer_cex", handlePosixPreferCex, false),
  add("klee_print_expr", handlePrintExpr, false),
  add("klee_print_range", handlePrintRange, false),
  add("klee_set_forking", handleSetForking, false),
  add("klee_stack_trace", handleStackTrace, false),
  add("klee_warning", handleWarning, false),
  add("klee_warning_once", handleWarningOnce, false),
  add("klee_alias_function", handleAliasFunction, false),
  add("malloc", handleMalloc, true),
  add("realloc", handleRealloc, true),



  // operator delete[](void*)
  add("_ZdaPv", handleDeleteArray, false),
  // operator delete(void*)
  add("_ZdlPv", handleDelete, false),

  // operator new[](unsigned int)
  add("_Znaj", handleNewArray, true),
  // operator new(unsigned int)
  add("_Znwj", handleNew, true),

  // FIXME-64: This is wrong for 64-bit long...

  // operator new[](unsigned long)
  add("_Znam", handleNewArray, true),
  // operator new(unsigned long)
  add("_Znwm", handleNew, true),

  // clang -fsanitize=unsigned-integer-overflow
  add("__ubsan_handle_add_overflow", handleAddOverflow, false),
  add("__ubsan_handle_sub_overflow", handleSubOverflow, false),
  add("__ubsan_handle_mul_overflow", handleMulOverflow, false),
  add("__ubsan_handle_divrem_overflow", handleDivRemOverflow, false),

#undef addDNR
#undef add  
};

SpecialFunctionHandler::const_iterator SpecialFunctionHandler::begin() {
  return SpecialFunctionHandler::const_iterator(handlerInfo);
}

SpecialFunctionHandler::const_iterator SpecialFunctionHandler::end() {
  // NULL pointer is sentinel
  return SpecialFunctionHandler::const_iterator(0);
}

SpecialFunctionHandler::const_iterator& SpecialFunctionHandler::const_iterator::operator++() {
  ++index;
  if ( index >= SpecialFunctionHandler::size())
  {
    // Out of range, return .end()
    base=0; // Sentinel
    index=0;
  }

  return *this;
}

int SpecialFunctionHandler::size() {
	return sizeof(handlerInfo)/sizeof(handlerInfo[0]);
}

SpecialFunctionHandler::SpecialFunctionHandler(Executor &_executor) 
  : executor(_executor) {}


void SpecialFunctionHandler::prepare() {
  unsigned N = size();

  for (unsigned i=0; i<N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);
    
    // No need to create if the function doesn't exist, since it cannot
    // be called in that case.
  
    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      // Make sure NoReturn attribute is set, for optimization and
      // coverage counting.
      if (hi.doesNotReturn)
        f->addFnAttr(Attribute::NoReturn);

      // Change to a declaration since we handle internally (simplifies
      // module and allows deleting dead code).
      if (!f->isDeclaration())
        f->deleteBody();
    }
  }
}

void SpecialFunctionHandler::bind() {
  unsigned N = sizeof(handlerInfo)/sizeof(handlerInfo[0]);

  for (unsigned i=0; i<N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);
    
    if (f && (!hi.doNotOverride || f->isDeclaration()))
      handlers[f] = std::make_pair(hi.handler, hi.hasReturnValue);
  }
}


bool SpecialFunctionHandler::handle(ExecutionState &state, 
                                    Function *f,
                                    KInstruction *target,
                                    std::vector< ref<Expr> > &arguments) {
  handlers_ty::iterator it = handlers.find(f);
  if (it != handlers.end()) {    
    Handler h = it->second.first;
    bool hasReturnValue = it->second.second;
     // FIXME: Check this... add test?
    if (!hasReturnValue && !target->inst->use_empty())
    {
      executor.terminateStateOnExecError(state, 
                                         "expected return value from void special function");
    }
    else
    {
      /*************************************************/
      /* OISH: this is where the malloc effect happens */
      /*************************************************/
      // llvm::errs() << "[OISH] Update malloc effect here" << "\n";
      (this->*h)(state, target, arguments);
    }
    return true;
  } else {
    return false;
  }
}

/****/

// reads a concrete string from memory
std::string 
SpecialFunctionHandler::readStringAtAddress(ExecutionState &state, 
                                            ref<Expr> addressExpr) {
  ObjectPair op;
  addressExpr = executor.toUnique(state, addressExpr);
  ref<ConstantExpr> address = cast<ConstantExpr>(addressExpr);
  if (!state.addressSpace.resolveOne(address, op))
    assert(0 && "XXX out of bounds / multiple resolution unhandled");
  bool res __attribute__ ((unused));
  assert(executor.solver->mustBeTrue(state, 
                                     EqExpr::create(address, 
                                                    op.first->getBaseExpr()),
                                     res) &&
         res &&
         "XXX interior pointer unhandled");
  const MemoryObject *mo = op.first;
  const ObjectState *os = op.second;

  char *buf = new char[mo->size];

  unsigned i;
  for (i = 0; i < mo->size - 1; i++) {
    ref<Expr> cur = os->read8(i);
    cur = executor.toUnique(state, cur);
    assert(isa<ConstantExpr>(cur) && 
           "hit symbolic char while reading concrete string");
    buf[i] = cast<ConstantExpr>(cur)->getZExtValue(8);
  }
  buf[i] = 0;
  
  std::string result(buf);
  delete[] buf;
  return result;
}

/****/

void SpecialFunctionHandler::handleAbort(ExecutionState &state,
                           KInstruction *target,
                           std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==0 && "invalid number of arguments to abort");
  executor.terminateStateOnError(state, "abort failure", Executor::Abort);
}

void SpecialFunctionHandler::handleExit(ExecutionState &state,
                           KInstruction *target,
                           std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to exit");
  executor.terminateStateOnExit(state);
}

void SpecialFunctionHandler::handleSilentExit(ExecutionState &state,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to exit");
  executor.terminateState(state);
}

void SpecialFunctionHandler::handleAliasFunction(ExecutionState &state,
						 KInstruction *target,
						 std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 && 
         "invalid number of arguments to klee_alias_function");
  std::string old_fn = readStringAtAddress(state, arguments[0]);
  std::string new_fn = readStringAtAddress(state, arguments[1]);
  KLEE_DEBUG_WITH_TYPE("alias_handling", llvm::errs() << "Replacing " << old_fn
                                           << "() with " << new_fn << "()\n");
  if (old_fn == new_fn)
    state.removeFnAlias(old_fn);
  else state.addFnAlias(old_fn, new_fn);
}

void SpecialFunctionHandler::handleAssert(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==3 && "invalid number of arguments to _assert");  
  executor.terminateStateOnError(state,
				 "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
				 Executor::Assert);
}

void SpecialFunctionHandler::handleAssertFail(ExecutionState &state,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==4 && "invalid number of arguments to __assert_fail");
  executor.terminateStateOnError(state,
				 "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
				 Executor::Assert);
}

void SpecialFunctionHandler::handleReportError(ExecutionState &state,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==4 && "invalid number of arguments to klee_report_error");
  
  // arguments[0], arguments[1] are file, line
  executor.terminateStateOnError(state,
				 readStringAtAddress(state, arguments[2]),
				 Executor::ReportError,
				 readStringAtAddress(state, arguments[3]).c_str());
}

void SpecialFunctionHandler::handleMerge(ExecutionState &state,
                           KInstruction *target,
                           std::vector<ref<Expr> > &arguments) {
  // nop
}

void SpecialFunctionHandler::handleNew(ExecutionState &state,
                         KInstruction *target,
                         std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to new");

  executor.executeAlloc(state, arguments[0], false, target);
}

void SpecialFunctionHandler::handleDelete(ExecutionState &state,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  // FIXME: Should check proper pairing with allocation type (malloc/free,
  // new/delete, new[]/delete[]).

  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to delete");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleNewArray(ExecutionState &state,
                              KInstruction *target,
                              std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to new[]");
  executor.executeAlloc(state, arguments[0], false, target);
}

void SpecialFunctionHandler::handleDeleteArray(ExecutionState &state,
                                 KInstruction *target,
                                 std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to delete[]");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleMalloc(ExecutionState &state,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 && "invalid number of arguments to malloc");
  executor.executeAlloc(state, arguments[0], false, target);
}

void SpecialFunctionHandler::handleAssume(ExecutionState &state,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to klee_assume");
  
  ref<Expr> e = arguments[0];
  
  if (e->getWidth() != Expr::Bool)
    e = NeExpr::create(e, ConstantExpr::create(0, e->getWidth()));
  
  bool res;
  bool success __attribute__ ((unused)) = executor.solver->mustBeFalse(state, e, res);
  assert(success && "FIXME: Unhandled solver failure");
  if (res) {
    if (SilentKleeAssume) {
      executor.terminateState(state);
    } else {
      executor.terminateStateOnError(state,
                                     "invalid klee_assume call (provably false)",
                                     Executor::User);
    }
  } else {
    executor.addConstraint(state, e);
  }
}

void SpecialFunctionHandler::handleIsSymbolic(ExecutionState &state,
                                KInstruction *target,
                                std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to klee_is_symbolic");

  executor.bindLocal(target, state, 
                     ConstantExpr::create(!isa<ConstantExpr>(arguments[0]),
                                          Expr::Int32));
}

void SpecialFunctionHandler::handlePreferCex(ExecutionState &state,
                                             KInstruction *target,
                                             std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_prefex_cex");

  ref<Expr> cond = arguments[1];
  if (cond->getWidth() != Expr::Bool)
    cond = NeExpr::create(cond, ConstantExpr::alloc(0, cond->getWidth()));

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "prefex_cex");
  
  assert(rl.size() == 1 &&
         "prefer_cex target must resolve to precisely one object");

  rl[0].first.first->cexPreferences.push_back(cond);
}

void SpecialFunctionHandler::handlePosixPreferCex(ExecutionState &state,
                                             KInstruction *target,
                                             std::vector<ref<Expr> > &arguments) {
  if (ReadablePosix)
    return handlePreferCex(state, target, arguments);
}

void SpecialFunctionHandler::handlePrintExpr(ExecutionState &state,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_print_expr");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  llvm::errs() << msg_str << ":" << arguments[1] << "\n";
}

void SpecialFunctionHandler::handleSetForking(ExecutionState &state,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_set_forking");
  ref<Expr> value = executor.toUnique(state, arguments[0]);
  
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    state.forkDisabled = CE->isZero();
  } else {
    executor.terminateStateOnError(state, 
                                   "klee_set_forking requires a constant arg",
                                   Executor::User);
  }
}

void SpecialFunctionHandler::handleStackTrace(ExecutionState &state,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  state.dumpStack(outs());
}

void SpecialFunctionHandler::handleWarning(ExecutionState &state,
                                           KInstruction *target,
                                           std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 && "invalid number of arguments to klee_warning");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning("%s: %s", state.stack.back().kf->function->getName().data(), 
               msg_str.c_str());
}

void SpecialFunctionHandler::handleWarningOnce(ExecutionState &state,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_warning_once");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning_once(0, "%s: %s", state.stack.back().kf->function->getName().data(),
                    msg_str.c_str());
}

void SpecialFunctionHandler::handlePrintRange(ExecutionState &state,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_print_range");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  llvm::errs() << msg_str << ":" << arguments[1];
  if (!isa<ConstantExpr>(arguments[1])) {
    // FIXME: Pull into a unique value method?
    ref<ConstantExpr> value;
    bool success __attribute__ ((unused)) = executor.solver->getValue(state, arguments[1], value);
    assert(success && "FIXME: Unhandled solver failure");
    bool res;
    success = executor.solver->mustBeTrue(state, 
                                          EqExpr::create(arguments[1], value), 
                                          res);
    assert(success && "FIXME: Unhandled solver failure");
    if (res) {
      llvm::errs() << " == " << value;
    } else { 
      llvm::errs() << " ~= " << value;
      std::pair< ref<Expr>, ref<Expr> > res =
        executor.solver->getRange(state, arguments[1]);
      llvm::errs() << " (in [" << res.first << ", " << res.second <<"])";
    }
  }
  llvm::errs() << "\n";
}

void SpecialFunctionHandler::handleGetObjSize(ExecutionState &state,
                                  KInstruction *target,
                                  std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_get_obj_size");
  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "klee_get_obj_size");
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    executor.bindLocal(
        target, *it->second,
        ConstantExpr::create(it->first.first->size,
                             executor.kmodule->targetData->getTypeSizeInBits(
                                 target->inst->getType())));
  }
}

void SpecialFunctionHandler::handleGetErrno(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==0 &&
         "invalid number of arguments to klee_get_errno");
  executor.bindLocal(target, state,
                     ConstantExpr::create(errno, Expr::Int32));
}

void SpecialFunctionHandler::handleCalloc(ExecutionState &state,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==2 &&
         "invalid number of arguments to calloc");

  ref<Expr> size = MulExpr::create(arguments[0],
                                   arguments[1]);
  executor.executeAlloc(state, size, false, target, true);
}

void SpecialFunctionHandler::handleRealloc(ExecutionState &state,
                            KInstruction *target,
                            std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==2 &&
         "invalid number of arguments to realloc");
  ref<Expr> address = arguments[0];
  ref<Expr> size = arguments[1];

  Executor::StatePair zeroSize = executor.fork(state, 
                                               Expr::createIsZero(size), 
                                               true);
  
  if (zeroSize.first) { // size == 0
    executor.executeFree(*zeroSize.first, address, target);   
  }
  if (zeroSize.second) { // size != 0
    Executor::StatePair zeroPointer = executor.fork(*zeroSize.second, 
                                                    Expr::createIsZero(address), 
                                                    true);
    
    if (zeroPointer.first) { // address == 0
      executor.executeAlloc(*zeroPointer.first, size, false, target);
    } 
    if (zeroPointer.second) { // address != 0
      Executor::ExactResolutionList rl;
      executor.resolveExact(*zeroPointer.second, address, rl, "realloc");
      
      for (Executor::ExactResolutionList::iterator it = rl.begin(), 
             ie = rl.end(); it != ie; ++it) {
        executor.executeAlloc(*it->second, size, false, target, false, 
                              it->first.second);
      }
    }
  }
}

void SpecialFunctionHandler::handleFree(ExecutionState &state,
                          KInstruction *target,
                          std::vector<ref<Expr> > &arguments) {
  // XXX should type check args
  assert(arguments.size()==1 &&
         "invalid number of arguments to free");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleCheckMemoryAccess(ExecutionState &state,
                                                     KInstruction *target,
                                                     std::vector<ref<Expr> > 
                                                       &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_check_memory_access");

  ref<Expr> address = executor.toUnique(state, arguments[0]);
  ref<Expr> size = executor.toUnique(state, arguments[1]);
  if (!isa<ConstantExpr>(address) || !isa<ConstantExpr>(size)) {
    executor.terminateStateOnError(state, 
                                   "check_memory_access requires constant args",
				   Executor::User);
  } else {
    ObjectPair op;

    if (!state.addressSpace.resolveOne(cast<ConstantExpr>(address), op)) {
      executor.terminateStateOnError(state,
                                     "check_memory_access: memory error",
				     Executor::Ptr, NULL,
                                     executor.getAddressInfo(state, address));
    } else {
      ref<Expr> chk = 
        op.first->getBoundsCheckPointer(address, 
                                        cast<ConstantExpr>(size)->getZExtValue());
      if (!chk->isTrue()) {
        executor.terminateStateOnError(state,
                                       "check_memory_access: memory error",
				       Executor::Ptr, NULL,
                                       executor.getAddressInfo(state, address));
      }
    }
  }
}



void SpecialFunctionHandler::handleBREAKPOINT(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
	int serial = ((llvm::ConstantInt *) (((llvm::CallInst *) target->inst)->getArgOperand(0)))->getZExtValue();
	fprintf(stdout,">> LIBOSIP BREAKPOINT [%d]\n",serial);
	if (serial == 320)
	{
		fprintf(stdout,"\n");
	}
}

void SpecialFunctionHandler::handleStrlen(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
  assert(arguments.size() == 1 && "Strlen can only have 1 argument");
  StrModel m = stringModel.modelStrlen(
                      executor.resolveOne(state,arguments[0]).second, 
                      arguments[0]);

  Executor::StatePair branches = executor.fork(state, m.second, true);
  ExecutionState *is_null_terminated = branches.first;
  ExecutionState *is_not_null_termianted = branches.second;
  if(is_not_null_termianted) {
      executor.terminateStateOnError(*is_not_null_termianted, 
          "String passed to strlen is not null terminated", Executor::Ptr);
  }
  executor.bindLocal(target,*is_null_terminated, m.first);
}


void SpecialFunctionHandler::handleStrcpy(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
  assert(arguments.size() == 2 && "Strcpy takes 2 arguments!");
  StrModel m = stringModel.modelStrcpy(
                      executor.resolveOne(state,arguments[0]).second, 
                      arguments[0],
                      executor.resolveOne(state,arguments[1]).second,
                      arguments[1]);

  Executor::StatePair branches = executor.fork(state, m.second, true);
  ExecutionState *src_is_NULL_terminated = branches.first;
  ExecutionState *src_NOT_NULL_terminated = branches.second;
  if(src_NOT_NULL_terminated) {
      executor.terminateStateOnError(*src_NOT_NULL_terminated, 
          "Trying to copy a not null terminated string", Executor::Ptr);
  }

  src_is_NULL_terminated->addConstraint(m.first);
}

void SpecialFunctionHandler::handleStrncmp(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
  assert(arguments.size() == 3 && "Strncmp takes 3 arguments!");
  state.dumpStack(errs());
  StrModel m = stringModel.modelStrncmp(
                      executor.resolveOne(state,arguments[0]).second, 
                      arguments[0],
                      executor.resolveOne(state,arguments[1]).second,
                      arguments[1],
						arguments[2]);

  Executor::StatePair branches = executor.fork(state, m.second, true);
  ExecutionState *not_access_after_end = branches.first;
  ExecutionState *access_after_end = branches.second;
  if(access_after_end) {
      executor.terminateStateOnError(*access_after_end, 
          "n in strncmp is bigger than sizes", Executor::Ptr);
  }
  executor.bindLocal(target,*not_access_after_end, m.first);
}

void SpecialFunctionHandler::handleStrncpy(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
  assert(arguments.size() == 3 && "Strncpy takes 3 arguments!");
  StrModel m = stringModel.modelStrncpy(
                      executor.resolveOne(state,arguments[0]).second, 
                      arguments[0],
                      executor.resolveOne(state,arguments[1]).second,
                      arguments[1],
						arguments[2]);

  Executor::StatePair branches = executor.fork(state, m.second, true);
  ExecutionState *not_access_after_end = branches.first;
  ExecutionState *access_after_end = branches.second;
  if(access_after_end) {
      executor.terminateStateOnError(*access_after_end, 
          "n in strncpy is bigger than sizes", Executor::Ptr);
  }

  not_access_after_end->addConstraint(m.first);
}

void SpecialFunctionHandler::handleStrcmp(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
  assert(arguments.size() == 2 && "Strcmp takes 2 arguments!");
  StrModel m = stringModel.modelStrcmp(
                      executor.resolveOne(state,arguments[0]).second, 
                      arguments[0],
                      executor.resolveOne(state,arguments[1]).second,
                      arguments[1]);

  Executor::StatePair branches = executor.fork(state, m.second, true);
  ExecutionState *p_q_NULL_terminated = branches.first;
  ExecutionState *p_q_NOT_NULL_terminated = branches.second;
  if(p_q_NOT_NULL_terminated) {
      executor.terminateStateOnError(*p_q_NOT_NULL_terminated, 
          "One of the strings in strcmp is not null terminated", Executor::Ptr);
  }

  if(!p_q_NULL_terminated) {
      klee_warning("P and q can't be null terminated");
      return;
  }


  executor.bindLocal(target,*p_q_NULL_terminated, m.first);
//  executor.executeStrcmp(state, target, arguments[0], arguments[1]);
}

void SpecialFunctionHandler::handleMyPrintOutput(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "mark string");
  
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    const ObjectState *os = it->first.second;
    std::string ab_name = os->getABSerial();

    errs() << "Looking up an ab serial " << ab_name << " size " << mo->size << "\n";
    ref<ConstantExpr> ce;
    bool success = executor.solver->getValue(state, StrVarExpr::create(ab_name.c_str()), ce);
    assert(success && "Failed to succesfully get value");
    std::string ab_value;
    ce->toString(ab_value, 1024);
    std::string s = "";
    // loop through all characters
    for(char c : ab_value)
    {
      if(isprint(c))
         s += c;
         else
         {
           std::stringstream stream;
           stream << std::hex << (unsigned int)(unsigned char)(c);
           std::string code = stream.str();
           s += std::string("\\x")+(code.size()<2?"0":"")+code;
        }
    }
    printf("%s: %s\n", ab_name.c_str(), s.c_str());
//    outs() << ab_name << ": " << s << "\n";
  }
}

static int numABSerials = 0;

void SpecialFunctionHandler::handleMarkString(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments) {

  assert(arguments.size() == 1 && "Mark symbolic only accepts a single pointer");

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "mark string");
  
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    MemoryObject *mo = const_cast<MemoryObject*>(it->first.first);
    ObjectState* wos = const_cast<ObjectState*>(it->first.second);
    wos->serial = ++numABSerials;
    wos->version = 0;
    std::string ab_name = wos->getABSerial();
    errs() << "Creating " << ab_name << " at:\n:";
    state.dumpStack(errs());
    if(mo->isGlobal) {
        //const GlobalVariable* v = dynamic_cast<const GlobalVariable*>(mo->allocSite);
        //assert(v && "Unsusported marks string of a non glbal variable global");
        //assert(v->isConstant() && "mark string of non constant global");
        char c[wos->size + 1];
        for(int i = 0; i < wos->size; i++) {
            c[i] = (char)dyn_cast<ConstantExpr>(wos->read8(i))->getZExtValue(8);
        }
        std::stringstream ss;
        ss << c << "\\x00";
        errs() << "Marking constant " << mo->name << " " << ss.str() << " os size: " << wos->size << "\n";
	      state.addConstraint(StrEqExpr::create(StrVarExpr::create(wos->getABSerial()), 
                                                      StrConstExpr::create(ss.str())));
    } else {

    errs() << "Creating an ab serial " << ab_name << " size " << mo->size << "\n";
	  state.addConstraint(EqExpr::create(
  		(StrLengthExpr::create(StrVarExpr::create(ab_name.c_str()))),
      mo->getIntSizeExpr()));
    }
    
  }



}

void SpecialFunctionHandler::handleStrrchr(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
	ref<Expr> x00 = StrConstExpr::create("\\x00");
	ref<Expr> one = BvToIntExpr::create(ConstantExpr::create(1,Expr::Int64));
	ref<Expr> zero= BvToIntExpr::create(ConstantExpr::create(0,Expr::Int64));
	ref<Expr> minusOne = SubExpr::create(zero,one);

	/***************************************************/
	/* [0] Make sure strrchr can only get 2 parameters */
	/***************************************************/
	assert(arguments.size() == 2 && "strrchr must have 2 arguments");
	
	/********************************/
	/* [1] Extract Object State ... */
	/********************************/
	const ObjectState* os = executor.resolveOne(state,arguments[0]).second;
	const MemoryObject* mos = os->getObject();

	/****************************************/
	/* [2] Extract s and c ... strrchr(s,c) */
	/****************************************/
	ref<Expr> s = arguments[0];
	ref<Expr> c = arguments[1];

	/*******************************/
	/* [3] Check if c appears in p */
	/*******************************/
	ref<Expr> size   = mos->getIntSizeExpr();
	ref<Expr> offset = mos->getOffsetExpr(s);	
	ref<Expr> p_var  = StrSubstrExpr::create(
		StrVarExpr::create(os->getABSerial()),
		BvToIntExpr::create(offset),
		SubExpr::create(size,offset));

	/************************************/
	/* [4] Build c as a length 1 String */
	/************************************/
	ref<Expr> c_as_length_1_string = StrFromBitVector8Expr::create(ExtractExpr::create(c,0,8));

	/*******************************/
	/* [5] Check if c appears in p */
	/*******************************/
	ref<Expr> firstIndexOfc   = StrFirstIdxOfExpr::create(p_var,c_as_length_1_string);
	ref<Expr> firstIndexOfx00 = StrFirstIdxOfExpr::create(p_var,x00);

	/*******************************/
	/* [6] Check if c appears in p */
	/*******************************/
	ref<Expr> c_appears_in_p = NotExpr::create(EqExpr::create(firstIndexOfc,minusOne));	
	ref<Expr> c_appears_in_p_before_x00 = SltExpr::create(firstIndexOfc,firstIndexOfx00);

	/*****************************************************************************/
	/* [7] Issue an error when invoking strrchr on a non NULL terminated string, */
	/*     and the specific char can be missing ...                              */
	/*****************************************************************************/
	ref<Expr> validAccess = AndExpr::create(
		OrExpr::create(
			NotExpr::create(EqExpr::create(firstIndexOfc,  minusOne)),
			NotExpr::create(EqExpr::create(firstIndexOfx00,minusOne))),
		mos->getBoundsCheckPointer(s));

	static int auxiliary_serial_index=0;
	/*********************************************/
	/* [8] Auxiliary string variables s1 and s2 */
	/*********************************************/
	std::string auxiliary_string_variable_s00 =
		std::string("AUX_STRING_")               +
		std::to_string(auxiliary_serial_index++) +
		std::string("_s00");
	ref<Expr> s00 = StrVarExpr::create(
		auxiliary_string_variable_s00);

	/*********************************************/
	/* [9] Auxiliary string variables s1 and s2 */
	/*********************************************/
	std::string auxiliary_string_variable_s1 =
		std::string("AUX_STRING_")               +
		std::to_string(auxiliary_serial_index++) +
		std::string("_s1");
	ref<Expr> s1 = StrVarExpr::create(
		auxiliary_string_variable_s1);

	/*********************************************/
	/* [10] Auxiliary string variables s1 and s2 */
	/*********************************************/
	std::string auxiliary_string_variable_s2 =
		std::string("AUX_STRING_")               +
		std::to_string(auxiliary_serial_index++) +
		std::string("_s2");
	ref<Expr> s2 = StrVarExpr::create(
		auxiliary_string_variable_s2);

	/*************************************************************/
	/* [11] Auxiliary constraints                                */
	/* --------------------------------------------------------- */
	/* (= s00 (str.substr p_var 0 (str.indexof p_var "\x00" 0))) */
	/*************************************************************/
	executor.addConstraint(state,
		StrEqExpr::create(
			StrSubstrExpr::create(
				p_var,
				zero,
				firstIndexOfx00),
			s00));
				
	/*******************************/
	/* [12] Auxiliary constraints  */
	/* --------------------------- */
	/* (str.suffixof s1 s00)       */
	/*******************************/
	executor.addConstraint(state,StrSuffixExpr::create(s00,s1));

	/*******************************/
	/* [13] Auxiliary constraints  */
	/* --------------------------- */
	/* (str.suffixof s2 s1)        */
	/*******************************/
	executor.addConstraint(state,StrSuffixExpr::create(s1,s2));

	/***************************************/
	/* [14] Auxiliary constraints          */
	/* ----------------------------------- */
	/* (= (str.len s1) (+ (str.len s2) 1)) */
	/***************************************/
	executor.addConstraint(state,
		EqExpr::create(
			StrLengthExpr::create(s1),
			AddExpr::create(
				StrLengthExpr::create(s2),
				one)));

	/*************************************/
	/* [15] Auxiliary constraints        */
	/* --------------------------------- */
	/* (or (and (str.contains s00 c)     */
	/*          (str.contains s1  c))    */
	/*     (not (str.contains s00 c)))   */
	/*************************************/
	executor.addConstraint(state,
		OrExpr::create(
			AndExpr::create(
				StrContainsExpr::create(s00,c),
				StrContainsExpr::create(s1, c)),
			NotExpr::create(
				StrContainsExpr::create(s00,c))));

	/*******************************/
	/* [16] Auxiliary constraints  */
	/* --------------------------- */
	/* (not (str.contains s2 c))   */
	/*******************************/
	executor.addConstraint(state,
		NotExpr::create(
			StrContainsExpr::create(s2,c)));
	
	/*************************/
	/* [17] return value ... */
	/*************************/
	ref<Expr> strrchrReturnValue = 
		SelectExpr::create(
			NotExpr::create(
				StrContainsExpr::create(s00,c)),
			zero,
			SubExpr::create(
				StrLengthExpr::create(s00),
				StrLengthExpr::create(s1)));
	
	/******************************************************/
	/* [18] From here on the same as strchr behaviour ... */
	/******************************************************/
	Executor::StatePair branches = executor.fork(state, validAccess, true);
	ExecutionState *invalid_access= branches.second;
	if (invalid_access)
	{
		executor.terminateStateOnError(
			*invalid_access,
			"Strrchr has out of bounds behaviour",
			Executor::Ptr);
	}

	/********************************/
	/* [19] Finally, bind local !!! */
	/********************************/
	executor.bindLocal(target,*(branches.first), strrchrReturnValue);
}

void SpecialFunctionHandler::handleStrchr(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
  bool isOnlyCompare = true;
  errs() << "Uses of strchr: \n";
  for(auto i = target->inst->use_begin(); i != target->inst->use_end(); i++) {
      i->print(errs());
      User* ui = *i;
      errs() << "\n";
      isOnlyCompare &= isa<CmpInst>(ui);
  }
  errs() << "Is only used in compare: " << isOnlyCompare << "\n";

  if(isOnlyCompare) {
    //Use contains semantics
  } else {
    //Use firstIdxOf semantics
  }


  StrModel m = stringModel.modelStrchr(
                      executor.resolveOne(state,arguments[0]).second, 
                      arguments[0],
                      arguments[1]);

  Executor::StatePair branches = executor.fork(state, m.second, true);
  ExecutionState *valid_access = branches.first;
  ExecutionState *invalid_access= branches.second;
  if(invalid_access) {
      executor.terminateStateOnError(*invalid_access, 
          "Strchr has out of bounds behaviour", Executor::Ptr);
  }

  executor.bindLocal(target,*valid_access, m.first);
}

void SpecialFunctionHandler::handleMyAtoi(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
}

void SpecialFunctionHandler::handleGetValue(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_get_value");

  executor.executeGetValue(state, arguments[0], target);
}

void SpecialFunctionHandler::handleDefineFixedObject(ExecutionState &state,
                                                     KInstruction *target,
                                                     std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==2 &&
         "invalid number of arguments to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[0]) &&
         "expect constant address argument to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[1]) &&
         "expect constant size argument to klee_define_fixed_object");
  
  uint64_t address = cast<ConstantExpr>(arguments[0])->getZExtValue();
  uint64_t size = cast<ConstantExpr>(arguments[1])->getZExtValue();
  MemoryObject *mo = executor.memory->allocateFixed(address, size, state.prevPC->inst);
  executor.bindObjectInState(state, mo, false);
  mo->isUserSpecified = true; // XXX hack;
}

void SpecialFunctionHandler::handleMakeSymbolic(ExecutionState &state,
                                                KInstruction *target,
                                                std::vector<ref<Expr> > &arguments) {
  std::string name;

  // FIXME: For backwards compatibility, we should eventually enforce the
  // correct arguments.
  if (arguments.size() == 2) {
    name = "unnamed";
  } else {
    // FIXME: Should be a user.err, not an assert.
    assert(arguments.size()==3 &&
           "invalid number of arguments to klee_make_symbolic");  
    name = readStringAtAddress(state, arguments[2]);
  }

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "make_symbolic");
  
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    mo->setName(name);
    
    const ObjectState *old = it->first.second;
    ExecutionState *s = it->second;
    
    if (old->readOnly) {
      executor.terminateStateOnError(*s, "cannot make readonly object symbolic",
                                     Executor::User);
      return;
    } 

    // FIXME: Type coercion should be done consistently somewhere.
    bool res;
    bool success __attribute__ ((unused)) =
      executor.solver->mustBeTrue(*s, 
                                  EqExpr::create(ZExtExpr::create(arguments[1],
                                                                  Context::get().getPointerWidth()),
                                                 mo->getSizeExpr()),
                                  res);
    assert(success && "FIXME: Unhandled solver failure");
    
    if (res) {
      executor.executeMakeSymbolic(*s, mo, name);
    } else {      
      executor.terminateStateOnError(*s, 
                                     "wrong size given to klee_make_symbolic[_name]", 
                                     Executor::User);
    }
  }
}

void SpecialFunctionHandler::handleMarkGlobal(ExecutionState &state,
                                              KInstruction *target,
                                              std::vector<ref<Expr> > &arguments) {
  assert(arguments.size()==1 &&
         "invalid number of arguments to klee_mark_global");  

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0], rl, "mark_global");
  
  for (Executor::ExactResolutionList::iterator it = rl.begin(), 
         ie = rl.end(); it != ie; ++it) {
    const MemoryObject *mo = it->first.first;
    assert(!mo->isLocal);
    mo->isGlobal = true;
  }
}

void SpecialFunctionHandler::handleAddOverflow(ExecutionState &state,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  executor.terminateStateOnError(state, "overflow on unsigned addition",
                                 Executor::Overflow);
}

void SpecialFunctionHandler::handleSubOverflow(ExecutionState &state,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  executor.terminateStateOnError(state, "overflow on unsigned subtraction",
                                 Executor::Overflow);
}

void SpecialFunctionHandler::handleMulOverflow(ExecutionState &state,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  executor.terminateStateOnError(state, "overflow on unsigned multiplication",
                                 Executor::Overflow);
}

void SpecialFunctionHandler::handleDivRemOverflow(ExecutionState &state,
                                               KInstruction *target,
                                               std::vector<ref<Expr> > &arguments) {
  executor.terminateStateOnError(state, "overflow on division or remainder",
                                 Executor::Overflow);
}

static int tmpStrVarSerialIdx=800;

void SpecialFunctionHandler::handle_da_loop_killer_f1(
	ExecutionState &state,
	KInstruction *target,
	std::vector<ref<Expr> > &arguments)
{
	/*************************************************/
	/* [1] Make sure f1 can only get 1 parameter ... */
	/*************************************************/
	assert(arguments.size() == 1 && "f1 can only have 1 argument");

	/********************************/
	/* [2] Extract Object State ... */
	/********************************/
	const ObjectState* os = executor.resolveOne(state,arguments[0]).second;
	const MemoryObject* mos = os->getObject();

	/***********************************/
	/* [3] Extract AB, offset and size */
	/***********************************/
	ref<Expr> AB = StrVarExpr::create(os->getABSerial());
	ref<Expr> offset = BvToIntExpr::create(mos->getOffsetExpr(arguments[0]));
	ref<Expr> size = SubExpr::create(mos->getIntSizeExpr(),offset);
	ref<Expr> s = StrSubstrExpr::create(AB,offset,size);
	
	/****************************************************/
	/* [4] Assemble pre + res to express the fact that: */
	/*     [-] pre does not contain "a" or "b"          */
	/*     [-] res[0] is not "a" nor "b"                */
	/****************************************************/
	ref<Expr> c   = StrVarExpr::create(std::string("c"  )+std::to_string(tmpStrVarSerialIdx++));
	ref<Expr> res = StrVarExpr::create(std::string("res")+std::to_string(tmpStrVarSerialIdx++));
	ref<Expr> pre = StrVarExpr::create(std::string("pre")+std::to_string(tmpStrVarSerialIdx++));

	/********************/
	/* [-] c is not "a" */
	/********************/
	executor.addConstraint(state,
		NotExpr::create(
			StrEqExpr::create(
				c,
				StrConstExpr::create("a"))));
				
	/********************/
	/* [-] c is not "b" */
	/********************/
	executor.addConstraint(state,
		NotExpr::create(
			StrEqExpr::create(
				c,
				StrConstExpr::create("b"))));

	/*********************/
	/* [-] c is length 1 */
	/*********************/
	executor.addConstraint(state,
		EqExpr::create(
			StrLengthExpr::create(c),
			ConstantExpr::create(1,Expr::Int64)));
	
	/******************************/
	/* [-] pre does not contain c */
	/******************************/
	executor.addConstraint(state,
		EqExpr::create(
			StrFirstIdxOfExpr::create(pre,c),
			ConstantExpr::create(-1,Expr::Int64)));

	/***********************/
	/* [-] s == pre ++ res */
	/***********************/
	executor.addConstraint(state,
		StrEqExpr::create(
			s,
			StrConcatExpr::create(pre,res)));

	/*************************/
	/* [-] res[0] is not "a" */
	/*************************/
	executor.addConstraint(state,
		NotExpr::create(
			StrEqExpr::create(
				StrCharAtExpr::create(res,ConstantExpr::create(0,Expr::Int64)),
				StrConstExpr::create("a"))));

	/*************************/
	/* [-] res[0] is not "b" */
	/*************************/
	executor.addConstraint(state,
		NotExpr::create(
			StrEqExpr::create(
				StrCharAtExpr::create(res,ConstantExpr::create(0,Expr::Int64)),
				StrConstExpr::create("b"))));

	/*****************************/
	/* [5] return s + strle(pre) */
	/*****************************/
	executor.bindLocal(target,state,
		AddExpr::create(
			arguments[0],
			StrLengthExpr::create(pre)));
}

