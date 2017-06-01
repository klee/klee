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

#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
#include "llvm/IR/Module.h"
#else
#include "llvm/IR/Module.h"
#endif
#include "llvm/ADT/Twine.h"

#if LLVM_VERSION_CODE <= LLVM_VERSION(3, 1)
#include "llvm/Target/TargetData.h"
#elif LLVM_VERSION_CODE <= LLVM_VERSION(3, 2)
#include "llvm/DataLayout.h"
#else
#include "llvm/IR/DataLayout.h"
#endif

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
cl::opt<bool>
symbolicFileIO("symbolicFileIO",cl::desc("Make File I/O symbolic, including Fopen,Fscanf,Fprintf,Fputs"),cl::init(false));


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
  add("fopen",handleOpen,true),
  add("fclose",handleClose,true),
  add("klee_make_IO_buffer",handleMakeIOBuffer,false),
  add("\01__isoc99_fscanf",handleFscanf,true),
  add("fscanf",handleFscanf,true),
  add("__isoc99_fscanf",handleFscanf,true),
  add("__isoc99_sscanf",handleSscanf,true),
  add("\01__isoc99_sscanf",handleSscanf,true),


  add("sscanf",handleSscanf,true),
  add("fprintf",handleFprintf,true),
  //add("printf",handleprintf,true),
  add("fputc",handleFputc,true),
  add("fread",handleFread,true),
  add("fwrite",handleFwrite,true),

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
    if(strcmp(hi.name,"fopen") == 0 ){
    	  if(!symbolicFileIO){
    		  handlerInfo[i] = {"",NULL,false,false,false};
    		  continue;
    	  }
    }
    if(strcmp(hi.name,"fscanf") == 0 ){
  	  if(!symbolicFileIO){
  		  handlerInfo[i] = {"",NULL,false,false,false};
  		  continue;
  	  }
    }
    if(strcmp(hi.name,"\01__isoc99_fscanf") == 0 ){
	  if(!symbolicFileIO){
		  handlerInfo[i] = {"",NULL,false,false,false};
		  continue;
	  }
    }
    if(strcmp(hi.name,"\01__isoc99_sscanf") == 0 ){
	  if(!symbolicFileIO){
		  handlerInfo[i] = {"",NULL,false,false,false};
		  continue;
	  }
    }
    if(strcmp(hi.name,"__isoc99_fscanf") == 0 ){
  	  if(!symbolicFileIO){
  		  handlerInfo[i] = {"",NULL,false,false,false};
  		  continue;
  	  }
    }
    if(strcmp(hi.name,"__isoc99_sscanf") == 0 ){
  	  if(!symbolicFileIO){
  		  handlerInfo[i] = {"",NULL,false,false,false};
  		  continue;
  	  }
    }
    if(strcmp(hi.name,"fputc") == 0 ){
   	  if(!symbolicFileIO){
   		  handlerInfo[i] = {"",NULL,false,false,false};
   		  continue;
   	  }
     }
    if(strcmp(hi.name,"fprintf") == 0 ){
	  if(!symbolicFileIO){
		  handlerInfo[i] = {"",NULL,false,false,false};
		  continue;
	  }
    }
    if(strcmp(hi.name,"printf") == 0 ){
	  if(!symbolicFileIO){
		  handlerInfo[i] = {"",NULL,false,false,false};
		  continue;
	  }
    }
    if(strcmp(hi.name,"fclose") == 0 ){
	  if(!symbolicFileIO){
		  handlerInfo[i] = {"",NULL,false,false,false};
		  continue;
	  }
    }
    if(strcmp(hi.name,"fread") == 0 ){
    	if(!symbolicFileIO){
  		  handlerInfo[i] = {"",NULL,false,false,false};
  		  continue;
    	}
    }
    if(strcmp(hi.name,"fwrite") == 0 ){
    	if(!symbolicFileIO){
  		  handlerInfo[i] = {"",NULL,false,false,false};
  		  continue;
    	}
    }
    if(strcmp(hi.name,"sscanf") == 0 ){
       	if(!symbolicFileIO){
   		  handlerInfo[i] = {"",NULL,false,false,false};
   		  continue;
     	}
    }

    Function *f = executor.kmodule->module->getFunction(hi.name);

    // No need to create if the function doesn't exist, since it cannot
    // be called in that case.
  
    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      // Make sure NoReturn attribute is set, for optimization and
      // coverage counting.
      if (hi.doesNotReturn)
#if LLVM_VERSION_CODE >= LLVM_VERSION(3, 3)
        f->addFnAttr(Attribute::NoReturn);
#elif LLVM_VERSION_CODE >= LLVM_VERSION(3, 2)
        f->addFnAttr(Attributes::NoReturn);
#else
        f->addFnAttr(Attribute::NoReturn);
#endif

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
    if (!hasReturnValue && !target->inst->use_empty()) {
      executor.terminateStateOnExecError(state, 
                                         "expected return value from void special function");
    } else {
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
  //Here because it is trying to get the size of Memory Object, an exact
  // match is needed. Gladtbx
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

/*
 * Gladtbx
 * Dummy Open function, lookup file name and setup file desc for
 * this file. Syntax: FileID|Buffer|ReadFlag|WriteFlag|Offset
 */
void SpecialFunctionHandler::handleOpen(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments) {
		if(arguments.size()!=2){
			klee_warning("Fopen Parameter Size Wrong");
			executor.terminateStateOnError(state,
			                                 "fopen parameter size wrong",
			                                 Executor::User);
		}
		Executor::ExactResolutionList rl;
		std::string name = readStringAtAddress(state,arguments[0]);
		std::string wr = readStringAtAddress(state,arguments[1]);
		klee_warning("Fopen file name: %s \n",name.c_str());
		klee_warning("Fopen file type: %s \n",wr.c_str());
		/*
		 * Lookup file in buffer
		 */
		std::pair<ObjectPair, int> buffer = state.lookupFile(name);
		assert(buffer.first.first && "File name not found, please use klee_make_IO_buffer to setup filename and buffer");

		int id = state.createFileDesc(buffer,wr);

		klee_warning("Fopen returned id: %d, size: %d\n",id,buffer.second);

		LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
		if (!resultType->isVoidTy()) {
			uint64_t v = id;
			unsigned width = resultType->getPrimitiveSizeInBits();
			ref<Expr> e = ConstantExpr::alloc(v,width);
			executor.bindLocal(target, state, e);
		 }
}

void SpecialFunctionHandler::handleClose(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments) {
	return;
}

void SpecialFunctionHandler::handleMakeIOBuffer(ExecutionState &state,
		        KInstruction *target,
		        std::vector<ref<Expr> > &arguments){
		assert(arguments.size()==2 && "Wrong number of parameters for klee_make_IO_buffer");
		std::string filename = readStringAtAddress(state, arguments[1]);
		Executor::ExactResolutionList rl;
		executor.resolveExact(state, arguments[0], rl, "mark_IO_buffer");
		//here arguments[0] is the array that is created as buffer. It must be the whole
		// array. Using partial array as buffer can be done how ever I don't see any necessity
		// of doing so. Gladtbx
	    for (Executor::ExactResolutionList::iterator it = rl.begin(),
		         ie = rl.end(); it != ie; ++it) {
	    	ObjectPair op = it->first;
	    	ExecutionState *s = it->second;
	    	std::pair<ObjectPair, int> mobuffer(op,op.first->size);
	    	std::pair<std::pair<ObjectPair, int>, std::string > entry(mobuffer,filename);
	    	s->addBuffer(entry);
	    }
}

uint64_t SpecialFunctionHandler::getValue(ExecutionState &state, ref<Expr> argument){
	ref<ConstantExpr> value;
	executor.solver->getValue(state,argument,value);
	return value.get()->getZExtValue();
}
/*
 * Process number
 */
std::vector<std::pair<ExecutionState*, ref<Expr> > > SpecialFunctionHandler::processNumber
(ExecutionState *current_state, std::vector<ref<Expr> > numberbuf, Expr::Width numwidth,int ary, bool neg){
	std::vector<std::pair<ExecutionState*, ref<Expr> > > stateNotProcessed;
	std::vector<std::pair<ExecutionState*, ref<Expr> > > stateProcessed;
	if(numberbuf.size()==0){
		return stateNotProcessed;
	}
	std::vector<ref<Expr> >::reverse_iterator bufferchar;

	ref<Expr> digit;
	int mul = 1;

	/*
	 * if it is a hex.. headache...
	 */
	if(ary == 16){
		ref<Expr> zero = ConstantExpr::create(0,numwidth);
		ref<Expr> sum;
		stateNotProcessed.push_back(std::pair<ExecutionState*, ref<Expr> >(current_state,zero));
		for(bufferchar = numberbuf.rbegin(); bufferchar!=numberbuf.rend();bufferchar++,mul = mul*ary){
			for(std::vector<std::pair<ExecutionState*, ref<Expr> > >::iterator s= stateNotProcessed.begin(); s != stateNotProcessed.end();s++){
				ref<Expr> lowerLetter = AndExpr::create(UleExpr::create(ConstantExpr::create('a',ConstantExpr::Int8),*bufferchar),
				UgeExpr::create(ConstantExpr::create('f',ConstantExpr::Int8),*bufferchar));
				Executor::StatePair lowerBranch = executor.fork(*(s->first), lowerLetter, true);//fork into new state, first -> true
				if(lowerBranch.first){//if it is a lower case ascii
					digit = SubExpr::create(*bufferchar,ConstantExpr::create(87,ConstantExpr::Int8));
					digit = ZExtExpr::create(digit,numwidth);
					sum = AddExpr::create(s->second,MulExpr::create(digit,ConstantExpr::create(mul,numwidth)));
					stateProcessed.push_back(std::pair<ExecutionState*, ref<Expr> > (lowerBranch.first,sum));
				}
				if(lowerBranch.second){
					ref<Expr> upperLetter = AndExpr::create(UleExpr::create(ConstantExpr::create('A',ConstantExpr::Int8),*bufferchar),
					UgeExpr::create(ConstantExpr::create('F',ConstantExpr::Int8),*bufferchar));
					Executor::StatePair upperBranch = executor.fork(*lowerBranch.second, upperLetter, true);//fork into new state, first -> true
					if(upperBranch.first){//if it is an upper ascii
						digit = SubExpr::create(*bufferchar,ConstantExpr::create(55,ConstantExpr::Int8));
						digit = ZExtExpr::create(digit,numwidth);
						sum = AddExpr::create(s->second,MulExpr::create(digit,ConstantExpr::create(mul,numwidth)));
						stateProcessed.push_back(std::pair<ExecutionState*, ref<Expr> > (upperBranch.first,sum));
					}
					if(upperBranch.second){//it is an integer(only option!)
						digit = SubExpr::create(*bufferchar,ConstantExpr::create(48,ConstantExpr::Int8));
						digit = ZExtExpr::create(digit,numwidth);
						sum = AddExpr::create(s->second,MulExpr::create(digit,ConstantExpr::create(mul,numwidth)));
						stateProcessed.push_back(std::pair<ExecutionState*, ref<Expr> > (upperBranch.second,sum));
					}
				}
			}
			//now switch buffer.
			stateNotProcessed.swap(stateProcessed);
			stateProcessed.clear();
		}
		if(neg){
			for(std::vector<std::pair<ExecutionState*, ref<Expr> > >::iterator s= stateNotProcessed.begin(); s != stateNotProcessed.end();s++){
				sum = SubExpr::create(ConstantExpr::create(0,numwidth),s->second);
				s->second = sum;
			}
		}
		return stateNotProcessed;
	}
	/*
	 * if it is octal or decimal
	 */
	ref<Expr> sum = ConstantExpr::create(0,numwidth);//Width here is for %d and %i
	for(bufferchar = numberbuf.rbegin(); bufferchar!=numberbuf.rend();bufferchar++){
		digit = SubExpr::create(*bufferchar,ConstantExpr::create(48,ConstantExpr::Int8));
		digit = ZExtExpr::create(digit,numwidth);
		sum = AddExpr::create(sum,MulExpr::create(digit,ConstantExpr::create(mul,numwidth)));
		mul= mul * ary;
	}
	if(neg){
		sum = SubExpr::create(ConstantExpr::create(0,numwidth),sum);
	}
	stateNotProcessed.push_back(std::pair<ExecutionState*, ref<Expr> >(current_state,sum));
	return stateNotProcessed;
}
/*
 * Function Pointer for generating conditions.
 * Conditions has to be generated based on each and every char read, so
 * better write functions to do them.
 */
ref<Expr> SpecialFunctionHandler::IntCondGen(ref<Expr> bufferchar){
	ref<Expr> left = UltExpr::create(ConstantExpr::create(47,ConstantExpr::Int8),bufferchar);
	ref<Expr> right = UgtExpr::create(ConstantExpr::create(58,ConstantExpr::Int8),bufferchar);
	ref<Expr> cond = AndExpr::create(left,right);
	return cond;
}

ref<Expr> SpecialFunctionHandler::OctCondGen(ref<Expr> bufferchar){
	ref<Expr> left = UltExpr::create(ConstantExpr::create(47,ConstantExpr::Int8),bufferchar);
	ref<Expr> right = UgtExpr::create(ConstantExpr::create(56,ConstantExpr::Int8),bufferchar);
	ref<Expr> cond = AndExpr::create(left,right);
	return cond;
}
/*
 * Function Pointer for generating conditions.
 * Conditions has to be generated based on each and every char read, so
 * better write functions to do them.
 */
ref<Expr> SpecialFunctionHandler::HexCondGen(ref<Expr> bufferchar){
	ref<Expr> left = UltExpr::create(ConstantExpr::create(47,ConstantExpr::Int8),bufferchar);
	ref<Expr> right = UgtExpr::create(ConstantExpr::create(58,ConstantExpr::Int8),bufferchar);
	ref<Expr> digit = AndExpr::create(left,right);
	ref<Expr> lowerLetter = AndExpr::create(UleExpr::create(ConstantExpr::create('a',ConstantExpr::Int8),bufferchar),
			UgeExpr::create(ConstantExpr::create('f',ConstantExpr::Int8),bufferchar));
	ref<Expr> upperLetter = AndExpr::create(UleExpr::create(ConstantExpr::create('A',ConstantExpr::Int8),bufferchar),
					UgeExpr::create(ConstantExpr::create('F',ConstantExpr::Int8),bufferchar));
	ref<Expr> cond = OrExpr::create(OrExpr::create(digit,lowerLetter),upperLetter);
	return cond;
}

void SpecialFunctionHandler::processScan(ExecutionState *current_state,Expr::Width w,ref<Expr> bufferchar,
			ref<Expr> targetBuf,const int fileid, const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed,
			KInstruction *target, int ary, ref<Expr> (*condFunc) (ref<Expr>)){
	/*
	 * Condition should be:
	 * if buffer lasts (size)
	 */
	while(current_state){
		/*
		 * 47 < digit < 58, determine if digit is a number in ascii
		 */
		ref<Expr> cond = condFunc(bufferchar);
		Executor::StatePair branches = executor.fork(*current_state, cond, true);//fork into new state, first -> true
		if(branches.second){//if it is not a digit
			std::vector<ref<Expr> > numberbuf = branches.second->ioBuffer.getBuffer();
			bool neg = branches.second->ioBuffer.getNeg();
			std::vector<std::pair<ExecutionState*, ref<Expr> > > result =  processNumber
			(branches.second, numberbuf, w, ary, neg);
			//ref<Expr> result = branches.second->ioBuffer.processNumber(w, ary);
			if(result.empty()){//if there is no digit read.
				//set the return value to be arguments read. Do not process any more.
				LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
				if (!resultType->isVoidTy()) {
					unsigned width = resultType->getPrimitiveSizeInBits();
					ref<Expr> e;
					if(branches.second->getBytesRead() == 0)
						e = ConstantExpr::alloc(EOF,width);
					else
						e = ConstantExpr::alloc(branches.second->getBytesRead(),width);
					executor.bindLocal(target, *branches.second, e);
				 }
			}
			else{
				for(std::vector<std::pair<ExecutionState*, ref<Expr> > >::iterator rs= result.begin(); rs != result.end();rs++){
					executor.executeMemoryOperation(*(rs->first),true,targetBuf,rs->second,0);//bind result with target
					rs->first->incBytesRead();
					ExecutionState::fileDesc* local_desc = rs->first->getBuffer(fileid);
					local_desc->decOffset();//We have read one more, now we need to back up;
					stateProcessed->push_back(rs->first);//put into statequeue.
				}
			}
		}
		if(branches.first){//if it is a digit.
			branches.first->ioBuffer.addDigit(bufferchar);//add digit to ExecutionState.
			ExecutionState::fileDesc* local_desc = branches.first->getBuffer(fileid);
			if(local_desc->getoffset() >= local_desc->getsize()){
				//return EOF
				LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
				if (!resultType->isVoidTy()) {
					unsigned width = resultType->getPrimitiveSizeInBits();
					ref<Expr> e;
					if(branches.first->getBytesRead() == 0)
						e = ConstantExpr::alloc(EOF,width);
					else
						e = ConstantExpr::alloc(branches.first->getBytesRead(),width);
					executor.bindLocal(target, *branches.first, e);
				 }
				return;//no more to read
			}
			bufferchar = op.second->read8(local_desc->getoffset());//read next char
			local_desc->incOffset();
		}
		current_state = branches.first;
	}
}

void SpecialFunctionHandler::processScanInt(ExecutionState *current_state,Expr::Width w,ref<Expr> bufferchar,
			ref<Expr> targetBuf,const int fileid, const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed, KInstruction *target){
		processScan(current_state, w, bufferchar, targetBuf, fileid, op, stateProcessed, target, 10, &IntCondGen);
}

void SpecialFunctionHandler::processScanChar(ExecutionState *current_state,ref<Expr> bufferchar,
			ref<Expr> targetBuf){
		executor.executeMemoryOperation(*current_state,true,targetBuf,bufferchar,0);//bind result with target
		current_state->incBytesRead();
}

void SpecialFunctionHandler::processScanOct(ExecutionState *current_state,Expr::Width w,ref<Expr> bufferchar,
			ref<Expr> targetBuf,const int fileid, const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed, KInstruction *target){
		processScan(current_state, w, bufferchar, targetBuf, fileid, op, stateProcessed, target, 8, &OctCondGen);
}

void SpecialFunctionHandler::processScanHex(ExecutionState *current_state,Expr::Width w,
		ref<Expr> bufferchar, ref<Expr> targetBuf, const int fileid,
		const ObjectPair& op, std::vector<ExecutionState*> *stateProcessed,
		KInstruction *target){
	/*
	 * If it is 0x, takes care of it
	 */
	ref<Expr> zeroEq = EqExpr::create(ConstantExpr::create('0',ConstantExpr::Int8),bufferchar);
	Executor::StatePair zeroBranch = executor.fork(*current_state, zeroEq, true);//fork into new state, first -> true
	if(zeroBranch.first){//if it is a zero at front
		ExecutionState::fileDesc* local_desc = zeroBranch.first->getBuffer(fileid);
		if(local_desc->getoffset() >= local_desc->getsize()){
			zeroBranch.first->ioBuffer.addDigit(bufferchar);
			//In this case, the previos read zero is the hex value, so we should bind it to the target.
			ref<Expr> result = zeroBranch.first->ioBuffer.processNumber(w, 16);//We don't need to check for Null because we just pushed.
			executor.executeMemoryOperation(*(zeroBranch.first),true,targetBuf,result,0);//bind result with target
			zeroBranch.first->incBytesRead();
			//return EOF as there is no more to read
			LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
			if (!resultType->isVoidTy()) {
				unsigned width = resultType->getPrimitiveSizeInBits();
				ref<Expr> e;
				e = ConstantExpr::alloc(zeroBranch.first->getBytesRead(),width);
				executor.bindLocal(target, *zeroBranch.first, e);
			 }
			//no more to read
		}
		else{//if there is more buffer to read, we read first
			bufferchar = op.second->read8(local_desc->getoffset());//read next char
			local_desc->incOffset();
			ref<Expr> xEq = OrExpr::create(EqExpr::create(ConstantExpr::create('x',ConstantExpr::Int8),bufferchar),
					EqExpr::create(ConstantExpr::create('X',ConstantExpr::Int8),bufferchar));//bufferchar== x||bufferchar==X
			Executor::StatePair xBranch = executor.fork(*zeroBranch.first, xEq, true);//fork into new state, first -> true
			if(xBranch.first){//if it is an x or X, so 0x or 0X in front
				ExecutionState::fileDesc* local_desc = zeroBranch.first->getBuffer(fileid);
				/*
				 * if it is an 0x or 0X header, we check if there is more buffer to read.
				 */
				if(local_desc->getoffset() >= local_desc->getsize()){
					//if no more buffer to read
					//return EOF
					LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
					if (!resultType->isVoidTy()) {
						unsigned width = resultType->getPrimitiveSizeInBits();
						ref<Expr> e;
						if(xBranch.first->getBytesRead() == 0)
							e = ConstantExpr::alloc(EOF,width);
						else
							e = ConstantExpr::alloc(xBranch.first->getBytesRead(),width);
						executor.bindLocal(target, *xBranch.first, e);
					 }
				}
				else{
					//there is more to read.
					bufferchar = op.second->read8(local_desc->getoffset());//read next char
					local_desc->incOffset();
					/*
					 * If digit is between 0-9 or a-f or A-F
					 */

					processScan(xBranch.first, w, bufferchar, targetBuf, fileid, op, stateProcessed, target, 16, &HexCondGen);
				}
			}
			if(xBranch.second){
				//if it is not followed by an x, we put this zero back to the digit pool of the state.
				xBranch.second->ioBuffer.addDigit(bufferchar);
				/*
				 * If digit is between 0-9 or a-f or A-F
				 */
				processScan(xBranch.second, w, bufferchar, targetBuf, fileid, op, stateProcessed, target, 16, &HexCondGen);
			}
		}
	}
	if(zeroBranch.second){
		//Easy case, no 0x or 0X header
		/*
		 * If digit is between 0-9 or a-f or A-F
		 */
		processScan(zeroBranch.second, w, bufferchar, targetBuf, fileid, op, stateProcessed, target, 16, &HexCondGen);
	}
}

void SpecialFunctionHandler::handleSscanf(ExecutionState &state,
		KInstruction *target,
        std::vector<ref<Expr> > &arguments){
		klee_warning("Sscanf");
		Executor::ExactResolutionList rl;
		std::string wr = "r";
		executor.resolveExact(state, arguments[0], rl, "mark_IO_buffer");
		//here arguments[0] is the array that is created as buffer. It must be the whole
		// array. Using partial array as buffer can be done how ever I don't see any necessity
		// of doing so. Gladtbx
		for (Executor::ExactResolutionList::iterator it = rl.begin(),
		         ie = rl.end(); it != ie; ++it) {
		   	ObjectPair op = it->first;
		   	ExecutionState *s = it->second;
		   	std::pair<ObjectPair, int> mobuffer(op,op.first->size);
		   	int id = s->createFileDesc(mobuffer,wr);
		   	ref<ConstantExpr> rid = ConstantExpr::create(id,ConstantExpr::Int32);
		   	std::vector<ref<Expr> > newargs(arguments);
		   	newargs[0] = rid;
		   	handleFscanf(*s,target,newargs);
		}
}

void SpecialFunctionHandler::handleFscanf(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments){
		assert(arguments.size()>1 && "Wrong number of arguments for fscanf");
		std::string format = readStringAtAddress(state,arguments[1]);
		ref<ConstantExpr> value;
		executor.solver->getValue(state,arguments[0],value);
		int fileid = value.get()->getZExtValue();
		//klee_warning("Fileid: %d", fileid);
		//klee_warning("Format String: %s", format.c_str());
		/*
		 * Start reading from the buffer
		 */
		ExecutionState::fileDesc* descriptor = state.getBuffer(fileid);
		ObjectPair op = descriptor->getBuffer();
		const MemoryObject* mo = op.first;
		int size = descriptor->getsize();
		std::string::iterator it;
		ref<ConstantExpr> bufferLocation = mo->getBaseExpr();

		/*
		 * First we resolve the content of the symbolic input.
		 * This may fork new states as different versions of input may be reached here.
		 * We do not need to resolve the buffers that are read to as it is going to be written any way
		 */
		Executor::ExactResolutionList rl;
		executor.resolveExact(state, bufferLocation, rl, "fscanf");
		//Here buffer location must be the starting point of the buffer memory object, so exact match
		// is needed. Gladtbx
		for (Executor::ExactResolutionList::iterator erit = rl.begin(),
				 erie = rl.end(); erit != erie; ++erit) {
			ObjectPair op = erit->first;
			ExecutionState *sinit = erit->second;
			sinit->clearBytesRead();//Clear bytes read counter
			sinit->ioBuffer.clear();//For each new scan, clear the ioBuffer.
			bool result;
			std::vector<ExecutionState*> stateNotProcessed;
			std::vector<ExecutionState*> stateProcessed;
			stateNotProcessed.push_back(sinit);
			/*
			 * After resolving symbolic input, we start reading one by one.
			 */
			for(it = format.begin();it!=format.end();it++){
				for(std::vector<ExecutionState*>::iterator s= stateNotProcessed.begin(); s != stateNotProcessed.end();s++){
					//for(every s in stateNotProcessed)
					descriptor = (*s)->getBuffer(fileid);
					if(descriptor->getoffset()>=size){
								//return EOF
						LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
						if (!resultType->isVoidTy()) {
							unsigned width = resultType->getPrimitiveSizeInBits();
							ref<Expr> e;
							if((*s)->getBytesRead() == 0)
								e = ConstantExpr::alloc(EOF,width);
							else
								e = ConstantExpr::alloc((*s)->getBytesRead(),width);
							executor.bindLocal(target, **s, e);
						 }
						continue;
					}
					//if we are not out of buffer, we read the content of the buffer.
					ref<Expr> bufferchar = op.second->read8(descriptor->getoffset());
					descriptor->incOffset();
					if(*it == '%'){//Read to dest
						//Remove all the spaces
						result = true;
						while (result) {
							ref<ConstantExpr> spacechar = ConstantExpr::create((uint64_t)' ',ConstantExpr::Int8);
							ref<ConstantExpr> tchar = ConstantExpr::create((uint64_t)'\t',ConstantExpr::Int8);
							ref<ConstantExpr> nchar = ConstantExpr::create((uint64_t)'\n',ConstantExpr::Int8);
							ref<ConstantExpr> vchar = ConstantExpr::create((uint64_t)'\v',ConstantExpr::Int8);
							ref<ConstantExpr> fchar = ConstantExpr::create((uint64_t)'\f',ConstantExpr::Int8);
							ref<ConstantExpr> rchar = ConstantExpr::create((uint64_t)'\r',ConstantExpr::Int8);
							ref<Expr> spaceeq = EqExpr::create(spacechar,bufferchar);
							ref<Expr> teq = EqExpr::create(tchar,bufferchar);
							ref<Expr> neq = EqExpr::create(nchar,bufferchar);
							ref<Expr> veq = EqExpr::create(vchar,bufferchar);
							ref<Expr> feq = EqExpr::create(fchar,bufferchar);
							ref<Expr> req = EqExpr::create(rchar,bufferchar);
							ref<Expr> firstor = OrExpr::create(spaceeq,teq);
							ref<Expr> secondor = OrExpr::create(neq,veq);
							ref<Expr> thirdor = OrExpr::create(feq,req);
							ref<Expr> lastor = OrExpr::create(OrExpr::create(firstor,secondor),thirdor);
							bool success =	executor.solver->mustBeTrue(**s,lastor,result);//Gladtbx: Must be true OR May be true, it is a question.
							assert(success && "fscanf solver failure");
							if(result){//a white character found, move to next char
								if(descriptor->getoffset()>=size){
									//return EOF
									LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
									if (!resultType->isVoidTy()) {
										unsigned width = resultType->getPrimitiveSizeInBits();
										ref<Expr> e;
										if((*s)->getBytesRead() == 0)
											e = ConstantExpr::alloc(EOF,width);
										else
											e = ConstantExpr::alloc((*s)->getBytesRead(),width);
										executor.bindLocal(target, **s, e);
									 }
									continue;
								}
								//if we are not out of buffer, we read the content of the buffer.
								bufferchar = op.second->read8(descriptor->getoffset());
								descriptor->incOffset();
							}
						}
						int argNum = (*s)->getBytesRead()+2;
						if(argNum >= arguments.size()){
							//Not enough parameters to be read to.
							klee_warning("Not enough parameter to be put char in");
							//set return to bytesread
							LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
							if (!resultType->isVoidTy()) {
								unsigned width = resultType->getPrimitiveSizeInBits();
								ref<Expr> e;
								if((*s)->getBytesRead() == 0)
									e = ConstantExpr::alloc(EOF,width);
								else
									e = ConstantExpr::alloc((*s)->getBytesRead(),width);
								executor.bindLocal(target, **s, e);
							 }
							continue;
						}
						ref<Expr> targetBuf = arguments[argNum];//how to write?

						/*
						 * We need to check if the dest buffer have the correct type.
						 */
						//it now is the first char of specifier
						char specifier[3]={'\0','\0','\0'};
						int index = 0;
						std::string::iterator nextit = it+1;//if it points out of bound?

						if(nextit == format.end()){
							//warning.Gladtbx:Maybe should be error?!
							klee_warning("Nothing behind %% sign!");
							continue;
						}
						while(*nextit == '0' ||*nextit == '1' ||*nextit == '2' ||*nextit == '3' ||*nextit == '4' ||*nextit == '5' ||*nextit == '6' ||*nextit == '7' ||*nextit == '8' ||*nextit == '9'){
							nextit++;//Fixme:Add support for numbers here;Gladtbx
							if(nextit == format.end()){
								//warning.Gladtbx:Maybe should be error?!
								klee_warning("Nothing behind %% sign!");
								continue;
							}
						}
						specifier[index] = *nextit;
						//first check if it is length. include'l','ll','L','h','hh','j','z','t'
						if(*nextit == 'l' || *nextit == 'h' || *nextit == 'j' || *nextit == 'z' || *nextit == 't' || *nextit == 'L'){
							nextit++;//add checking for it not out of bound
							if(nextit == format.end()){
								klee_warning("Fscanf missing indicator before reaching end of string");
								continue;
								//return
							}
							index++;
							specifier[index] = *nextit;
							if(specifier[0] == specifier[1]){
								//we have 'hh' or 'll' and still need one more char
								nextit++;
								index++;
								specifier[index] = *nextit;
							}
						}

						//clear neg flag and previous read data
						(*s)->ioBuffer.clear();
						//if no length is found we have the specifier we need
						//check %f
						/*
						 * %d || %i
						 */
						if(specifier[0] == 'd' || specifier[0] == 'o' ){
							//check negative sign
							ref<Expr> cond = EqExpr::create(ConstantExpr::create((uint64_t) '-',ConstantExpr::Int8), bufferchar);
							Executor::StatePair signbranches = executor.fork(**s, cond, true);//fork into new state, first -> true
							if(signbranches.first){//if negative
								signbranches.first->ioBuffer.setneg();
								ExecutionState::fileDesc* local_desc = signbranches.first->getBuffer(fileid);
								if(local_desc->getoffset() >= size){
									//return EOF
									LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
									if (!resultType->isVoidTy()) {
										unsigned width = resultType->getPrimitiveSizeInBits();
										ref<Expr> e;
										if((signbranches.first)->getBytesRead() == 0)
											e = ConstantExpr::alloc(EOF,width);
										else
											e = ConstantExpr::alloc((signbranches.first)->getBytesRead(),width);
										executor.bindLocal(target, *signbranches.first, e);
									 }
								}
								bufferchar = op.second->read8(local_desc->getoffset());//read next char
								local_desc->incOffset();
								if(specifier[0] == 'd')
									processScanInt(signbranches.first,ConstantExpr::Int32,bufferchar,targetBuf,fileid,op,&stateProcessed,target);
								else
									processScanOct(signbranches.first,ConstantExpr::Int32,bufferchar,targetBuf,fileid,op,&stateProcessed,target);
							}

							if(signbranches.second){//if positive
								if(specifier[0] == 'd')
									processScanInt(signbranches.second,ConstantExpr::Int32,bufferchar,targetBuf,fileid,op,&stateProcessed,target);
								else
									processScanOct(signbranches.second,ConstantExpr::Int32,bufferchar,targetBuf,fileid,op,&stateProcessed,target);
							}
							//bytesread++;
						}
						else if(specifier[0] == 'i'){

						}
						else if(specifier[0] == 'c'){
							processScanChar(*s,bufferchar,targetBuf);
							stateProcessed.push_back(*s);
							//bytesread++;
						}
						else if(specifier[0] == 'x'){
							//check negative sign
							ref<Expr> cond = EqExpr::create(ConstantExpr::create((uint64_t) '-',ConstantExpr::Int8), bufferchar);
							Executor::StatePair signbranches = executor.fork(**s, cond, true);//fork into new state, first -> true
							if(signbranches.first){//if negative
								signbranches.first->ioBuffer.setneg();
								ExecutionState::fileDesc* local_desc = signbranches.first->getBuffer(fileid);
								if(local_desc->getoffset() >= size){
									//return EOF
									LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
									if (!resultType->isVoidTy()) {
										unsigned width = resultType->getPrimitiveSizeInBits();
										ref<Expr> e;
										if(signbranches.first->getBytesRead() == 0)
											e = ConstantExpr::alloc(EOF,width);
										else
											e = ConstantExpr::alloc(signbranches.first->getBytesRead(),width);
										executor.bindLocal(target, *signbranches.first, e);
									 }
								}
								bufferchar = op.second->read8(local_desc->getoffset());//read next char
								local_desc->incOffset();
								processScanHex(signbranches.first,ConstantExpr::Int32,bufferchar,targetBuf,fileid,op,&stateProcessed,target);
							}

							if(signbranches.second){//if positive
								processScanHex(signbranches.second,ConstantExpr::Int32,bufferchar,targetBuf,fileid,op,&stateProcessed,target);
							}
						}
						else if(specifier[0] == 'u'){

						}
						else if(specifier[0] == 's'){

						}
						//else{
							//if not a correct specifier, return numbytes read;
							LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
							if (!resultType->isVoidTy()) {
								unsigned width = resultType->getPrimitiveSizeInBits();
								ref<Expr> e;
								if((*s)->getBytesRead() == 0)
									e = ConstantExpr::alloc(EOF,width);
								else
									e = ConstantExpr::alloc((*s)->getBytesRead(),width);
								executor.bindLocal(target, **s, e);
							//}
						}
					}

					else if(*it =='\t'||*it == '\n' ||*it == '\v' || *it== '\f' || *it == '\r' || *it == ' '){//add tab space etc..
						//if((it+1)==format.end()){//if last white space, we need to get rid of all white spaces after current pointer in file
							result = true;
							while (result) {
								ref<ConstantExpr> spacechar = ConstantExpr::create((uint64_t)' ',ConstantExpr::Int8);
								ref<ConstantExpr> tchar = ConstantExpr::create((uint64_t)'\t',ConstantExpr::Int8);
								ref<ConstantExpr> nchar = ConstantExpr::create((uint64_t)'\n',ConstantExpr::Int8);
								ref<ConstantExpr> vchar = ConstantExpr::create((uint64_t)'\v',ConstantExpr::Int8);
								ref<ConstantExpr> fchar = ConstantExpr::create((uint64_t)'\f',ConstantExpr::Int8);
								ref<ConstantExpr> rchar = ConstantExpr::create((uint64_t)'\r',ConstantExpr::Int8);
								ref<Expr> spaceeq = EqExpr::create(spacechar,bufferchar);
								ref<Expr> teq = EqExpr::create(tchar,bufferchar);
								ref<Expr> neq = EqExpr::create(nchar,bufferchar);
								ref<Expr> veq = EqExpr::create(vchar,bufferchar);
								ref<Expr> feq = EqExpr::create(fchar,bufferchar);
								ref<Expr> req = EqExpr::create(rchar,bufferchar);
								ref<Expr> firstor = OrExpr::create(spaceeq,teq);
								ref<Expr> secondor = OrExpr::create(neq,veq);
								ref<Expr> thirdor = OrExpr::create(feq,req);
								ref<Expr> lastor = OrExpr::create(OrExpr::create(firstor,secondor),thirdor);
								bool success =	executor.solver->mustBeTrue(**s,lastor,result);//Gladtbx: Must be true OR May be true, it is a question.
								assert(success && "fscanf solver failure");
								if(result){//a white character found, move to next char
									if(descriptor->getoffset()>=size){
										//return EOF
										LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
										if (!resultType->isVoidTy()) {
											unsigned width = resultType->getPrimitiveSizeInBits();
											ref<Expr> e;
											if((*s)->getBytesRead() == 0)
												e = ConstantExpr::alloc(EOF,width);
											else
												e = ConstantExpr::alloc((*s)->getBytesRead(),width);
											executor.bindLocal(target, **s, e);
										 }
										continue;
									}
									//if we are not out of buffer, we read the content of the buffer.
									bufferchar = op.second->read8(descriptor->getoffset());
									descriptor->incOffset();
								}
							}
						//}
						descriptor->decOffset();//Because we have already incremented, and now it is a skip, we need to avoid over reading...
						stateProcessed.push_back(*s);
					}
					else{
						ref<ConstantExpr> formatchar = ConstantExpr::create((uint64_t)*it,ConstantExpr::Int8);
						bool success =	executor.solver->mustBeTrue(**s,EqExpr::create(formatchar,bufferchar),result);//Gladtbx: Must be true OR May be true, it is a question.
						assert(success && "fscanf solver failure");
						if(result){
							//a match can be found so we move on...

							stateProcessed.push_back(*s);
						}
						else{
							klee_warning("Fscanf found string not able to match!!");
							//return num of chars read.
							LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
							if (!resultType->isVoidTy()) {
								unsigned width = resultType->getPrimitiveSizeInBits();
								ref<Expr> e;
								if((*s)->getBytesRead() == 0)
									e = ConstantExpr::alloc(EOF,width);
								else
									e = ConstantExpr::alloc((*s)->getBytesRead(),width);
								executor.bindLocal(target, **s, e);
							 }
						}
					}
				}
				if(*it == '%'){//if it is percent, we need it++
					it++;
					if(it == format.end()){
						break;
					}
					if((*it == 'l' || *it == 'h' || *it == 'j' || *it == 'z' || *it == 't' || *it == 'L')){
						it++;//FIXME:Only consider 1 bit width indicator.
						if(it== format.end()){
							break;
						}
					}
				}
				//now switch buffer.
				stateNotProcessed.swap(stateProcessed);
				stateProcessed.clear();
			}
			//after processed every literal in the format string, return bytes read for each state inside not processed_state.
			for(std::vector<ExecutionState*>::iterator s= stateNotProcessed.begin(); s != stateNotProcessed.end();s++){
				LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
				if (!resultType->isVoidTy()) {
					unsigned width = resultType->getPrimitiveSizeInBits();
					ref<Expr> e;
					if((*s)->getBytesRead() == 0)
						e = ConstantExpr::alloc(EOF,width);
					else
						e = ConstantExpr::alloc((*s)->getBytesRead(),width);
					executor.bindLocal(target, **s, e);
				 }
			}
		}
}

void SpecialFunctionHandler::handleprintf(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments){
	std::vector<ref<Expr> > newargs(arguments);
	ref<ConstantExpr> zero = ConstantExpr::create(0,ConstantExpr::Int32);
	newargs.insert(newargs.begin(),zero);
	handleFprintf(state,target,newargs);
}

void SpecialFunctionHandler::handleFprintf(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments){
	assert(arguments.size()>1 && "Wrong number of arguments for fprintf");
	std::string format = readStringAtAddress(state,arguments[1]);
	ref<ConstantExpr> value;
	executor.solver->getValue(state,arguments[0],value);
	int fileid = value.get()->getZExtValue();
	//klee_warning("Fileid: %d", fileid);
	//klee_warning("Format String: %s", format.c_str());
	/*
	 * Start reading from the buffer
	 */
	ExecutionState::fileDesc* descriptor = state.getBuffer(fileid);
	ObjectPair op = descriptor->getBuffer();
	const MemoryObject* mo = op.first;
	int size = descriptor->getsize();
	std::string::iterator it;
	ref<ConstantExpr> bufferLocation = mo->getBaseExpr();
	/*
	 * First we resolve the content of the symbolic input.
	 * This may fork new states as different versions of input may be reached here.
	 * We do not need to resolve the buffers that are read to as it is going to be written any way
	 */
	int byteswrite = 0;
	for(it = format.begin();it!=format.end();it++){
		if(*it != '%'){//not a variable
			if(descriptor->getoffset()>=size){//too much to put into the target buffer
				klee_error("Targetbuffer overflow, check Fprintf or allocate larger buffer");
			}
			ref<Expr> writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
			ref<Expr> writtenChar = ConstantExpr::create(*it,ConstantExpr::Int8);
			executor.executeMemoryOperation(state,true,writtenLoc,writtenChar,0);
			descriptor->incOffset();
			if(descriptor->getoffset()>=descriptor->getsize()){
				klee_error("Output buffer over flow!!");
			}
		}
		else{
			it++;
			if(it == format.end()){
				klee_warning("Missing indicator after %% sign in fprintf!");
				ref<Expr> writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
				ref<Expr> writtenChar = ConstantExpr::create('%',ConstantExpr::Int8);
				executor.executeMemoryOperation(state,true,writtenLoc,writtenChar,0);
				descriptor->incOffset();
				if(descriptor->getoffset()>=descriptor->getsize()){
					klee_error("Output buffer over flow!!");
				}
				continue;
			}
			if(*it == 'd' || *it == 'x' || *it == 'X' || *it == 'o'){//if 32 bit width
				//check if argument is the correct width
				ref<Expr> character = ZExtExpr::create(arguments[byteswrite+2],Expr::Int32);
				ref<Expr> writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
				executor.executeMemoryOperation(state,true,writtenLoc,character,0);
				byteswrite++;
				descriptor->incOffset();
				descriptor->incOffset();
				descriptor->incOffset();
				descriptor->incOffset();
				if(descriptor->getoffset()>=descriptor->getsize()){
					klee_error("Output buffer over flow!!");
				}
			}
			else if(*it == 'c'){//if 8 bit width
				//check if argument is the correct width
				ref<Expr> character = ZExtExpr::create(arguments[byteswrite+2],Expr::Int8);
				ref<Expr> writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
				executor.executeMemoryOperation(state,true,writtenLoc,character,0);
				byteswrite++;
				descriptor->incOffset();
				if(descriptor->getoffset()>=descriptor->getsize()){
					klee_error("Output buffer over flow!!");
				}
			}
			else{
				ref<Expr> writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
				ref<Expr> writtenChar = ConstantExpr::create('%',ConstantExpr::Int8);
				executor.executeMemoryOperation(state,true,writtenLoc,writtenChar,0);
				descriptor->incOffset();
				if(descriptor->getoffset()>=descriptor->getsize()){
					klee_error("Output buffer over flow!!");
				}
				writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
				writtenChar = ConstantExpr::create(*it,ConstantExpr::Int8);
				executor.executeMemoryOperation(state,true,writtenLoc,writtenChar,0);
				descriptor->incOffset();
				if(descriptor->getoffset()>=descriptor->getsize()){
					klee_error("Output buffer over flow!!");
				}
			}
		}
	}
	LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
	if (!resultType->isVoidTy()) {
		unsigned width = resultType->getPrimitiveSizeInBits();
		ref<Expr> e;
		e = ConstantExpr::alloc(byteswrite,width);
		executor.bindLocal(target, state, e);
	 }
}


void SpecialFunctionHandler::handleFputc(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments){
	assert(arguments.size() == 2 && "Wrong number of arguments for fprintf");
	ref<ConstantExpr> value;
	executor.solver->getValue(state,arguments[1],value);
	int fileid = value.get()->getZExtValue();
	klee_warning("Fileid: %d", fileid);
	/*
	 * Start reading from the buffer
	 */
	ExecutionState::fileDesc* descriptor = state.getBuffer(fileid);
	ObjectPair op = descriptor->getBuffer();
	std::string::iterator it;
	const MemoryObject* mo = op.first;
	ref<ConstantExpr> bufferLocation = mo->getBaseExpr();

	if(arguments[0]->getWidth()!=Expr::Int32){
		klee_error("Fputc target buffer not correct type!");
	}
	ref<Expr> character = ZExtExpr::create(arguments[0],Expr::Int8);
	ref<Expr> writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
	executor.executeMemoryOperation(state,true,writtenLoc,character,0);
	descriptor->incOffset();
	if(descriptor->getoffset()>=descriptor->getsize()){
		klee_error("Output buffer over flow!!");
	}
	LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
	if (!resultType->isVoidTy()) {
		executor.bindLocal(target, state, arguments[0]);
	 }
}

void SpecialFunctionHandler::handleFread(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments){
		assert(arguments.size()==4 && "Wrong number of arguments for fscanf");
		ref<ConstantExpr> value;
		executor.solver->getValue(state,arguments[3],value);
		int fileid = value.get()->getZExtValue();
		executor.solver->getValue(state,arguments[2],value);
		int count = value.get()->getZExtValue();
		executor.solver->getValue(state,arguments[1],value);
		int size = value.get()->getZExtValue();
		ref<Expr> targetBuf = arguments[0];
		ExecutionState::fileDesc* descriptor = state.getBuffer(fileid);
		ObjectPair op = descriptor->getBuffer();

		const MemoryObject* mo = op.first;
		ref<ConstantExpr> bufferLocation = mo->getBaseExpr();
		Executor::ExactResolutionList rl;
		executor.resolveExact(state, bufferLocation, rl, "fread");
		//Here buffer location must be the starting point of the buffer memory object, so exact match
		// is needed. Gladtbx
		for (Executor::ExactResolutionList::iterator erit = rl.begin(),
				 erie = rl.end(); erit != erie; ++erit) {
			ObjectPair op = erit->first;
			int desc_size = descriptor->getsize();
			int bytesRead = 0;
			const ObjectState* os = op.second;
			ref<Expr> buffer;
			for(int i = 0; i < count; i++){
				switch (size){
				case 1:
					if(descriptor->getoffset() >= desc_size){//eof reached, number of bytes read returned;
						LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
						if (!resultType->isVoidTy()) {
							unsigned width = resultType->getPrimitiveSizeInBits();
							ref<Expr> e;
							e = ConstantExpr::alloc(bytesRead,width);
							executor.bindLocal(target, state, e);
						 }
						return;
					}
					buffer = os->read8(descriptor->getoffset());
					descriptor->incOffset();
					bytesRead++;
					executor.executeMemoryOperation(state,true,targetBuf,buffer,0);//bind result with target
					targetBuf = AddExpr::create(targetBuf,ConstantExpr::create(1,targetBuf->getWidth()));//targetBuf++
					break;
				case 2:
					if(descriptor->getoffset()+1>=desc_size){
						LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
						if (!resultType->isVoidTy()) {
							unsigned width = resultType->getPrimitiveSizeInBits();
							ref<Expr> e;
							e = ConstantExpr::alloc(bytesRead,width);
							executor.bindLocal(target, state, e);
						 }
						return;
					}
					buffer = os->read(descriptor->getoffset(),Expr::Int16);
					descriptor->incOffset();
					descriptor->incOffset();
					bytesRead+=2;
					executor.executeMemoryOperation(state,true,targetBuf,buffer,0);//bind result with target
					targetBuf = AddExpr::create(targetBuf,ConstantExpr::create(2,targetBuf->getWidth()));//targetBuf++
					break;
				case 4:
					if(descriptor->getoffset()+1>=desc_size){
						LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
						if (!resultType->isVoidTy()) {
							unsigned width = resultType->getPrimitiveSizeInBits();
							ref<Expr> e;
							e = ConstantExpr::alloc(bytesRead,width);
							executor.bindLocal(target, state, e);
						 }
						return;
					}
					buffer = os->read(descriptor->getoffset(),Expr::Int32);
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					bytesRead+=4;
					executor.executeMemoryOperation(state,true,targetBuf,buffer,0);//bind result with target
					targetBuf = AddExpr::create(targetBuf,ConstantExpr::create(4,targetBuf->getWidth()));//targetBuf++
					break;
				case 8:
					if(descriptor->getoffset()+7>=desc_size){
						LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
						if (!resultType->isVoidTy()) {
							unsigned width = resultType->getPrimitiveSizeInBits();
							ref<Expr> e;
							e = ConstantExpr::alloc(bytesRead,width);
							executor.bindLocal(target, state, e);
						 }
						return;
					}
					buffer = os->read(descriptor->getoffset(),Expr::Int64);
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					descriptor->incOffset();
					bytesRead+=8;
					executor.executeMemoryOperation(state,true,targetBuf,buffer,0);//bind result with target
					targetBuf = AddExpr::create(targetBuf,ConstantExpr::create(8,targetBuf->getWidth()));//targetBuf++
					break;
				default:
					if(descriptor->getoffset()+size-1>=desc_size){
						LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
						if (!resultType->isVoidTy()) {
							unsigned width = resultType->getPrimitiveSizeInBits();
							ref<Expr> e;
							e = ConstantExpr::alloc(bytesRead,width);
							executor.bindLocal(target, state, e);
						 }
						return;
					}
					klee_warning("Fread Warning: Not reading size 1 2 4 or 8, may not be supported!");
					buffer = os->read(descriptor->getoffset(),size);
					for(int j = 0; j < size; j++){
						descriptor->incOffset();
					}
					bytesRead+=size;
					executor.executeMemoryOperation(state,true,targetBuf,buffer,0);//bind result with target
					targetBuf = AddExpr::create(targetBuf,ConstantExpr::create(size,targetBuf->getWidth()));//targetBuf++
					break;
				}
			}
		}
}

void SpecialFunctionHandler::handleFwrite(ExecutionState &state,
        KInstruction *target,
        std::vector<ref<Expr> > &arguments){
		assert(arguments.size()==4 && "Wrong number of arguments for fwrite");
		ref<ConstantExpr> value;
		executor.solver->getValue(state,arguments[3],value);
		int fileid = value.get()->getZExtValue();
		executor.solver->getValue(state,arguments[2],value);
		int count = value.get()->getZExtValue();
		executor.solver->getValue(state,arguments[1],value);
		int size = value.get()->getZExtValue();
		ref<Expr> address = arguments[0];
		/*
		 * Here we copy part of the load instruction from the Executor.cpp to read from the
		 * pointer bufferLocation.
		 */

		bool inBounds;
		std::vector<std::pair<std::pair<ObjectPair, ref<Expr> >, ExecutionState*> > workpool;
		ResolutionList rl;
		unsigned bytes = Expr::getMinBytesForWidth(Expr::Int8);//For fwrite, we always do byte by byte write.
		ObjectPair op;
		bool success;
		executor.solver->setTimeout(executor.coreSolverTimeout);
		if (!state.addressSpace.resolveOne(state, executor.solver, address, op, success)) {
			address = executor.toConstant(state, address, "resolveOne failure");
		  success = state.addressSpace.resolveOne(cast<ConstantExpr>(address), op);
		}
		executor.solver->setTimeout(0);
		if (success) {
			const MemoryObject *mo = op.first;

//			if (MaxSymArraySize && mo->size>=MaxSymArraySize) {
//				address = executor.toConstant(state, address, "max-sym-array-size");
//			}

			ref<Expr> offset = mo->getOffsetExpr(address);

			executor.solver->setTimeout(executor.coreSolverTimeout);
			bool success = executor.solver->mustBeTrue(state,
					mo->getBoundsCheckOffset(offset, bytes),
					inBounds);
			executor.solver->setTimeout(0);
			if (!success) {
				state.pc = state.prevPC;
				executor.terminateStateEarly(state, "Query timed out (bounds check).");
				return;
			}

			if(inBounds){
				workpool.push_back(std::pair<std::pair <ObjectPair, ref<Expr> >, ExecutionState*> (
					std::pair<ObjectPair, ref<Expr> >(op,offset), &state));
			}
		}

		//If resolve not successful or out of bounds resolve
		if(!success || !inBounds){
			executor.solver->setTimeout(executor.coreSolverTimeout);
			bool incomplete = state.addressSpace.resolve(state, executor.solver, address, rl,
					0, executor.coreSolverTimeout);
			executor.solver->setTimeout(0);

			// XXX there is some query wasteage here. who cares?
			ExecutionState *unbound = &state;

			for (ResolutionList::iterator i = rl.begin(), ie = rl.end(); i != ie; ++i) {
				const MemoryObject *mo = i->first;
				ref<Expr> offset = mo->getOffsetExpr(address);
				ref<Expr> inBounds = mo->getBoundsCheckPointer(address, bytes);

				Executor::StatePair branches = executor.fork(*unbound, inBounds, true);
				ExecutionState *bound = branches.first;
				if(bound){
					workpool.push_back(std::pair<std::pair <ObjectPair, ref<Expr> >, ExecutionState*> (
										std::pair<ObjectPair, ref<Expr> >(*i,offset), bound));
				}

				// bound can be 0 on failure or overlapped
				unbound = branches.second;
				if (!unbound)
					break;
			}

			// XXX should we distinguish out of bounds and overlapped cases?
			if (unbound) {
				if (incomplete) {
					executor.terminateStateEarly(*unbound, "Query timed out (resolve).");
				} else {
					executor.terminateStateOnError(*unbound,
							"memory error: out of bound pointer",
							Executor::Ptr
							);
				}
			}
		}

		for (std::vector<std::pair<std::pair<ObjectPair, ref<Expr> >, ExecutionState*> >::iterator opit = workpool.begin(),
				ie = workpool.end(); opit != ie; ++opit) {
			ExecutionState::fileDesc* descriptor = opit->second->getBuffer(fileid);
			int desc_size = descriptor->getsize();
			int bytesWritten = 0;
			ref<Expr> offset = opit->first.second;
			ref<ConstantExpr> bufferLocation = descriptor->getBuffer().first->getBaseExpr();


			for(int i = 0; i < count * size; i++){
				if(descriptor->getoffset() >= desc_size){//eof reached, number of bytes read returned;
					LLVM_TYPE_Q llvm::Type *resultType = target->inst->getType();
					if (!resultType->isVoidTy()) {
						unsigned width = resultType->getPrimitiveSizeInBits();
						ref<Expr> e;
						e = ConstantExpr::alloc(bytesWritten,width);
						executor.bindLocal(target, *(opit->second), e);
					 }
					return;
				}
				ref<Expr> addOffset = AddExpr::create(offset,ConstantExpr::create(i,offset->getWidth()));
				ref<Expr> tbw = opit->first.first.second->read(addOffset,Expr::Int8);//tbw = to be written
				ref<Expr> writtenLoc = AddExpr::create(bufferLocation,ConstantExpr::create(descriptor->getoffset(),bufferLocation->getWidth()));
				executor.executeMemoryOperation(*(opit->second),true,writtenLoc,tbw,0);//bind result with target
				descriptor->incOffset();
			}
		}
}
