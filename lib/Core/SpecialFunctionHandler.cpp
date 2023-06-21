//===-- SpecialFunctionHandler.cpp ----------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SpecialFunctionHandler.h"

#include "ExecutionState.h"
#include "Executor.h"
#include "Memory.h"
#include "MemoryManager.h"
#include "Searcher.h"
#include "StatsTracker.h"
#include "TimingSolver.h"
#include "TypeManager.h"

#include "klee/Config/config.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Solver/SolverCmdLine.h"
#include "klee/Support/Casting.h"
#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"
#include "klee/klee.h"

#include "llvm/ADT/Twine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Format.h"

#include <cerrno>
#include <sstream>
#include <time.h>

using namespace llvm;
using namespace klee;

namespace {
cl::opt<bool>
    ReadablePosix("readable-posix-inputs", cl::init(false),
                  cl::desc("Prefer creation of POSIX inputs (command-line "
                           "arguments, files, etc.) with human readable bytes. "
                           "Note: option is expensive when creating lots of "
                           "tests (default=false)"),
                  cl::cat(TestGenCat));

cl::opt<bool>
    SilentKleeAssume("silent-klee-assume", cl::init(false),
                     cl::desc("Silently terminate paths with an infeasible "
                              "condition given to klee_assume() rather than "
                              "emitting an error (default=false)"),
                     cl::cat(TerminationCat));

cl::opt<bool> CheckOutOfMemory("check-out-of-memory", cl::init(false),
                               cl::desc("Enable out-of-memory checking during "
                                        "memory allocation (default=false)"),
                               cl::cat(ExecCat));
} // namespace

/// \todo Almost all of the demands in this file should be replaced
/// with terminateState calls.

///

// FIXME: We are more or less committed to requiring an intrinsic
// library these days. We can move some of this stuff there,
// especially things like realloc which have complicated semantics
// w.r.t. forking. Among other things this makes delayed query
// dispatch easier to implement.
static SpecialFunctionHandler::HandlerInfo handlerInfo[] = {
#define add(name, handler, ret)                                                \
  { name, &SpecialFunctionHandler::handler, false, ret, false }
#define addDNR(name, handler)                                                  \
  { name, &SpecialFunctionHandler::handler, true, false, false }
    addDNR("__assert_rtn", handleAssertFail),
    addDNR("__assert_fail", handleAssertFail),
    addDNR("__assert", handleAssertFail),
    addDNR("_assert", handleAssert),
    addDNR("abort", handleAbort),
    addDNR("_exit", handleExit),
    {"exit", &SpecialFunctionHandler::handleExit, true, false, true},
    addDNR("klee_abort", handleAbort),
    addDNR("klee_silent_exit", handleSilentExit),
    addDNR("klee_report_error", handleReportError),
    add("calloc", handleCalloc, true),
    add("free", handleFree, false),
    add("klee_assume", handleAssume, false),
    add("klee_sleep", handleSleep, false),
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
#ifndef __APPLE__
    add("__errno_location", handleErrnoLocation, true),
#else
    add("__error", handleErrnoLocation, true),
#endif
    add("klee_is_symbolic", handleIsSymbolic, true),
    add("klee_make_symbolic", handleMakeSymbolic, false),
    add("klee_mark_global", handleMarkGlobal, false),
    add("klee_prefer_cex", handlePreferCex, false),
    add("klee_posix_prefer_cex", handlePosixPreferCex, false),
    add("klee_print_expr", handlePrintExpr, false),
    add("klee_print_range", handlePrintRange, false),
    add("klee_set_forking", handleSetForking, false),
    add("klee_stack_trace", handleStackTrace, false),
    add("klee_warning", handleWarning, false),
    add("klee_warning_once", handleWarningOnce, false),
    add("malloc", handleMalloc, true),
    add("memalign", handleMemalign, true),
    add("realloc", handleRealloc, true),

#ifdef SUPPORT_KLEE_EH_CXX
    add("_klee_eh_Unwind_RaiseException_impl", handleEhUnwindRaiseExceptionImpl,
        false),
    add("klee_eh_typeid_for", handleEhTypeid, true),
#endif

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
    // operator new[](unsigned long, std::nothrow_t const&)
    add("_ZnamRKSt9nothrow_t", handleNewNothrowArray, true),
    // operator new(unsigned long)
    add("_Znwm", handleNew, true),

    // Run clang with -fsanitize=undefined or any its subset of checks
    // https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#available-checks
    add("__ubsan_handle_type_mismatch_v1", handleTypeMismatchV1, false),
    add("__ubsan_handle_type_mismatch_v1_abort", handleTypeMismatchV1, false),
    add("__ubsan_handle_alignment_assumption", handleAlignmentAssumption,
        false),
    add("__ubsan_handle_alignment_assumption_abort", handleAlignmentAssumption,
        false),
    add("__ubsan_handle_add_overflow", handleAddOverflow, false),
    add("__ubsan_handle_add_overflow_abort", handleAddOverflow, false),
    add("__ubsan_handle_sub_overflow", handleSubOverflow, false),
    add("__ubsan_handle_sub_overflow_abort", handleSubOverflow, false),
    add("__ubsan_handle_mul_overflow", handleMulOverflow, false),
    add("__ubsan_handle_mul_overflow_abort", handleMulOverflow, false),
    add("__ubsan_handle_negate_overflow", handleNegateOverflow, false),
    add("__ubsan_handle_negate_overflow_abort", handleNegateOverflow, false),
    add("__ubsan_handle_divrem_overflow", handleDivRemOverflow, false),
    add("__ubsan_handle_divrem_overflow_abort", handleDivRemOverflow, false),
    add("__ubsan_handle_shift_out_of_bounds", handleShiftOutOfBounds, false),
    add("__ubsan_handle_shift_out_of_bounds_abort", handleShiftOutOfBounds,
        false),
    add("__ubsan_handle_out_of_bounds", handleOutOfBounds, false),
    add("__ubsan_handle_out_of_bounds_abort", handleOutOfBounds, false),
    add("__ubsan_handle_builtin_unreachable", handleBuiltinUnreachable, false),
    add("__ubsan_handle_missing_return", handleMissingReturn, false),
    add("__ubsan_handle_vla_bound_not_positive", handleVlaBoundNotPositive,
        false),
    add("__ubsan_handle_vla_bound_not_positive_abort",
        handleVlaBoundNotPositive, false),
    add("__ubsan_handle_float_cast_overflow", handleFloatCastOverflow, false),
    add("__ubsan_handle_float_cast_overflow_abort", handleFloatCastOverflow,
        false),
    add("__ubsan_handle_load_invalid_value", handleLoadInvalidValue, false),
    add("__ubsan_handle_load_invalid_value_abort", handleLoadInvalidValue,
        false),
    add("__ubsan_handle_implicit_conversion", handleImplicitConversion, false),
    add("__ubsan_handle_implicit_conversion_abort", handleImplicitConversion,
        false),
    add("__ubsan_handle_invalid_builtin", handleInvalidBuiltin, false),
    add("__ubsan_handle_invalid_builtin_abort", handleInvalidBuiltin, false),
    add("__ubsan_handle_nonnull_return_v1", handleNonnullReturnV1, false),
    add("__ubsan_handle_nonnull_return_v1_abort", handleNonnullReturnV1, false),
    add("__ubsan_handle_nullability_return_v1", handleNullabilityReturnV1,
        false),
    add("__ubsan_handle_nullability_return_v1_abort", handleNullabilityReturnV1,
        false),
    add("__ubsan_handle_nonnull_arg", handleNonnullArg, false),
    add("__ubsan_handle_nonnull_arg_abort", handleNonnullArg, false),
    add("__ubsan_handle_nullability_arg", handleNullabilityArg, false),
    add("__ubsan_handle_nullability_arg_abort", handleNullabilityArg, false),
    add("__ubsan_handle_pointer_overflow", handlePointerOverflow, false),
    add("__ubsan_handle_pointer_overflow_abort", handlePointerOverflow, false),

    // float classification instrinsics
    add("klee_is_nan_float", handleIsNaN, true),
    add("klee_is_nan_double", handleIsNaN, true),
    add("klee_is_nan_long_double", handleIsNaN, true),
    add("klee_is_infinite_float", handleIsInfinite, true),
    add("klee_is_infinite_double", handleIsInfinite, true),
    add("klee_is_infinite_long_double", handleIsInfinite, true),
    add("klee_is_normal_float", handleIsNormal, true),
    add("klee_is_normal_double", handleIsNormal, true),
    add("klee_is_normal_long_double", handleIsNormal, true),
    add("klee_is_subnormal_float", handleIsSubnormal, true),
    add("klee_is_subnormal_double", handleIsSubnormal, true),
    add("klee_is_subnormal_long_double", handleIsSubnormal, true),

    // Rounding mode intrinsics
    add("klee_get_rounding_mode", handleGetRoundingMode, true),
    add("klee_set_rounding_mode_internal", handleSetConcreteRoundingMode,
        false),

    // square root
    add("klee_sqrt_float", handleSqrt, true),
    add("klee_sqrt_double", handleSqrt, true),
#if defined(__x86_64__) || defined(__i386__)
    add("klee_sqrt_long_double", handleSqrt, true),
#endif

    // floating point absolute
    add("klee_abs_float", handleFAbs, true),
    add("klee_abs_double", handleFAbs, true),
#if defined(__x86_64__) || defined(__i386__)
    add("klee_abs_long_double", handleFAbs, true),
#endif
    add("klee_rint", handleRint, true),
    add("klee_rintf", handleRint, true),
#if defined(__x86_64__) || defined(__i386__)
    add("klee_rintl", handleRint, true),
#endif
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

SpecialFunctionHandler::const_iterator &
SpecialFunctionHandler::const_iterator::operator++() {
  ++index;
  if (index >= SpecialFunctionHandler::size()) {
    // Out of range, return .end()
    base = 0; // Sentinel
    index = 0;
  }

  return *this;
}

int SpecialFunctionHandler::size() {
  return sizeof(handlerInfo) / sizeof(handlerInfo[0]);
}

SpecialFunctionHandler::SpecialFunctionHandler(Executor &_executor)
    : executor(_executor) {}

void SpecialFunctionHandler::prepare(
    std::vector<const char *> &preservedFunctions) {
  unsigned N = size();

  for (unsigned i = 0; i < N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);

    // No need to create if the function doesn't exist, since it cannot
    // be called in that case.
    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      preservedFunctions.push_back(hi.name);
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
  unsigned N = sizeof(handlerInfo) / sizeof(handlerInfo[0]);

  for (unsigned i = 0; i < N; ++i) {
    HandlerInfo &hi = handlerInfo[i];
    Function *f = executor.kmodule->module->getFunction(hi.name);

    if (f && (!hi.doNotOverride || f->isDeclaration())) {
      handlers[f] = std::make_pair(hi.handler, hi.hasReturnValue);

      if (executor.kmodule->functionMap.count(f)) {
        executor.kmodule->functionMap.at(f)->kleeHandled = true;
      }
    }
  }
}

bool SpecialFunctionHandler::handle(ExecutionState &state, Function *f,
                                    KInstruction *target,
                                    std::vector<ref<Expr>> &arguments) {
  handlers_ty::iterator it = handlers.find(f);
  if (it != handlers.end()) {
    Handler h = it->second.first;
    bool hasReturnValue = it->second.second;
    // FIXME: Check this... add test?
    if (!hasReturnValue && !target->inst->use_empty()) {
      executor.terminateStateOnExecError(
          state, "expected return value from void special function");
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
std::string SpecialFunctionHandler::readStringAtAddress(ExecutionState &state,
                                                        ref<Expr> addressExpr) {
  IDType idStringAddress;
  addressExpr = executor.toUnique(state, addressExpr);
  if (!isa<ConstantExpr>(addressExpr)) {
    executor.terminateStateOnUserError(
        state, "Symbolic string pointer passed to one of the klee_ functions");
    return "";
  }
  ref<ConstantExpr> address = cast<ConstantExpr>(addressExpr);
  if (!state.addressSpace.resolveOne(
          address, executor.typeSystemManager->getUnknownType(),
          idStringAddress)) {
    executor.terminateStateOnUserError(
        state, "Invalid string pointer passed to one of the klee_ functions");
    return "";
  }
  ObjectPair op = state.addressSpace.findObject(idStringAddress);
  const MemoryObject *mo = op.first;
  const ObjectState *os = op.second;

  auto relativeOffset = mo->getOffsetExpr(address);
  // the relativeOffset must be concrete as the address is concrete
  size_t offset = cast<ConstantExpr>(relativeOffset)->getZExtValue();

  std::ostringstream buf;
  char c = 0;
  for (size_t i = offset; i < mo->size; ++i) {
    ref<Expr> cur = os->read8(i);
    cur = executor.toUnique(state, cur);
    assert(isa<ConstantExpr>(cur) &&
           "hit symbolic char while reading concrete string");
    c = cast<ConstantExpr>(cur)->getZExtValue(8);
    if (c == '\0') {
      // we read the whole string
      break;
    }

    buf << c;
  }

  if (c != '\0') {
    klee_warning_once(0, "String not terminated by \\0 passed to "
                         "one of the klee_ functions");
  }

  return buf.str();
}

/****/

void SpecialFunctionHandler::handleAbort(ExecutionState &state,
                                         KInstruction *target,
                                         std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 0 && "invalid number of arguments to abort");
  executor.terminateStateOnError(state, "abort failure",
                                 StateTerminationType::Abort);
}

void SpecialFunctionHandler::handleExit(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to exit");
  executor.terminateStateOnExit(state);
}

void SpecialFunctionHandler::handleSilentExit(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to exit");
  executor.terminateStateEarly(state, "", StateTerminationType::SilentExit);
}

void SpecialFunctionHandler::handleAssert(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 3 && "invalid number of arguments to _assert");
  executor.terminateStateOnError(
      state, "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
      StateTerminationType::Assert);
}

void SpecialFunctionHandler::handleAssertFail(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 4 &&
         "invalid number of arguments to __assert_fail");
  executor.terminateStateOnError(
      state, "ASSERTION FAIL: " + readStringAtAddress(state, arguments[0]),
      StateTerminationType::Assert);
}

void SpecialFunctionHandler::handleReportError(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 4 &&
         "invalid number of arguments to klee_report_error");

  // arguments[0,1,2,3] are file, line, message, suffix
  executor.terminateStateOnError(
      state, readStringAtAddress(state, arguments[2]),
      StateTerminationType::ReportError, "",
      readStringAtAddress(state, arguments[3]).c_str());
}

void SpecialFunctionHandler::handleNew(ExecutionState &state,
                                       KInstruction *target,
                                       std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to new");
  executor.executeAlloc(state, arguments[0], false, target,
                        executor.typeSystemManager->handleAlloc(arguments[0]),
                        false, nullptr, 0, CheckOutOfMemory);
}

void SpecialFunctionHandler::handleDelete(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // FIXME: Should check proper pairing with allocation type (malloc/free,
  // new/delete, new[]/delete[]).

  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to delete");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleNewArray(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to new[]");
  executor.executeAlloc(state, arguments[0], false, target,
                        executor.typeSystemManager->handleAlloc(arguments[0]),
                        false, nullptr, 0, CheckOutOfMemory);
}

void SpecialFunctionHandler::handleNewNothrowArray(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 &&
         "invalid number of arguments to new[](unsigned long, std::nothrow_t "
         "const&)");
  executor.executeAlloc(state, arguments[0], false, target,
                        executor.typeSystemManager->handleAlloc(arguments[0]),
                        false, nullptr, 0, CheckOutOfMemory);
}

void SpecialFunctionHandler::handleDeleteArray(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to delete[]");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleMalloc(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to malloc");
  executor.executeAlloc(state, arguments[0], false, target,
                        executor.typeSystemManager->handleAlloc(arguments[0]),
                        false, nullptr, 0, CheckOutOfMemory);
}

void SpecialFunctionHandler::handleMemalign(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  if (arguments.size() != 2) {
    executor.terminateStateOnUserError(
        state, "Incorrect number of arguments to memalign(size_t alignment, "
               "size_t size)");
    return;
  }

  std::pair<ref<Expr>, ref<Expr>> alignmentRangeExpr =
      executor.solver->getRange(state.constraints.cs(), arguments[0],
                                state.queryMetaData);
  ref<Expr> alignmentExpr = alignmentRangeExpr.first;
  auto alignmentConstExpr = dyn_cast<ConstantExpr>(alignmentExpr);

  if (!alignmentConstExpr) {
    executor.terminateStateOnUserError(
        state, "Could not determine size of symbolic alignment");
    return;
  }

  uint64_t alignment = alignmentConstExpr->getZExtValue();

  // Warn, if the expression has more than one solution
  if (alignmentRangeExpr.first != alignmentRangeExpr.second) {
    klee_warning_once(
        0, "Symbolic alignment for memalign. Choosing smallest alignment");
  }

  executor.executeAlloc(state, arguments[1], false, target,
                        executor.typeSystemManager->handleAlloc(arguments[1]),
                        false, 0, alignment);
}

#ifdef SUPPORT_KLEE_EH_CXX
void SpecialFunctionHandler::handleEhUnwindRaiseExceptionImpl(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to _klee_eh_Unwind_RaiseException_impl");

  ref<ConstantExpr> exceptionObject = dyn_cast<ConstantExpr>(arguments[0]);
  if (!exceptionObject.get()) {
    executor.terminateStateOnExecError(
        state, "Internal error: Symbolic exception pointer");
    return;
  }

  if (isa_and_nonnull<SearchPhaseUnwindingInformation>(
          state.unwindingInformation.get())) {
    executor.terminateStateOnExecError(
        state,
        "Internal error: Unwinding restarted during an ongoing search phase");
    return;
  }

  state.unwindingInformation =
      std::make_unique<SearchPhaseUnwindingInformation>(exceptionObject,
                                                        state.stack.size() - 1);

  executor.unwindToNextLandingpad(state);
}

void SpecialFunctionHandler::handleEhTypeid(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_eh_typeid_for");

  executor.bindLocal(target, state, executor.getEhTypeidFor(arguments[0]));
}
#endif // SUPPORT_KLEE_EH_CXX

void SpecialFunctionHandler::handleSleep(ExecutionState &state,
                                         KInstruction *target,
                                         std::vector<ref<Expr>> &arguments) {
  nanosleep((const struct timespec[]){{1, 0}}, NULL);
}

void SpecialFunctionHandler::handleAssume(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to klee_assume");

  ref<Expr> e = arguments[0];

  if (e->getWidth() != Expr::Bool)
    e = NeExpr::create(e, ConstantExpr::create(0, e->getWidth()));

  bool res;
  bool success __attribute__((unused)) = executor.solver->mustBeFalse(
      state.constraints.cs(), e, res, state.queryMetaData);
  assert(success && "FIXME: Unhandled solver failure");
  if (res) {
    if (SilentKleeAssume) {
      executor.terminateState(state, StateTerminationType::SilentExit);
    } else {
      executor.terminateStateOnUserError(
          state, "invalid klee_assume call (provably false)");
    }
  } else {
    executor.addConstraint(state, e);
  }
}

void SpecialFunctionHandler::handleIsSymbolic(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_is_symbolic");

  executor.bindLocal(
      target, state,
      ConstantExpr::create(!isa<ConstantExpr>(arguments[0]), Expr::Int32));
}

void SpecialFunctionHandler::handlePreferCex(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_prefex_cex");

  ref<Expr> cond = arguments[1];
  if (cond->getWidth() != Expr::Bool)
    cond = NeExpr::create(cond, ConstantExpr::alloc(0, cond->getWidth()));

  state.addCexPreference(cond);
}

void SpecialFunctionHandler::handlePosixPreferCex(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  if (ReadablePosix)
    return handlePreferCex(state, target, arguments);
}

void SpecialFunctionHandler::handlePrintExpr(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_print_expr");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  llvm::errs() << msg_str << ":" << arguments[1] << "\n";
}

void SpecialFunctionHandler::handleSetForking(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_set_forking");
  ref<Expr> value = executor.toUnique(state, arguments[0]);

  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(value)) {
    state.forkDisabled = CE->isZero();
  } else {
    executor.terminateStateOnUserError(
        state, "klee_set_forking requires a constant arg");
  }
}

void SpecialFunctionHandler::handleStackTrace(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  state.dumpStack(outs());
}

void SpecialFunctionHandler::handleWarning(ExecutionState &state,
                                           KInstruction *target,
                                           std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_warning");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning("%s: %s", state.stack.back().kf->function->getName().data(),
               msg_str.c_str());
}

void SpecialFunctionHandler::handleWarningOnce(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_warning_once");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  klee_warning_once(0, "%s: %s",
                    state.stack.back().kf->function->getName().data(),
                    msg_str.c_str());
}

void SpecialFunctionHandler::handlePrintRange(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_print_range");

  std::string msg_str = readStringAtAddress(state, arguments[0]);
  llvm::errs() << msg_str << ":" << arguments[1];
  if (!isa<ConstantExpr>(arguments[1])) {
    // FIXME: Pull into a unique value method?
    ref<ConstantExpr> value;
    bool success __attribute__((unused)) = executor.solver->getValue(
        state.constraints.cs(), arguments[1], value, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    bool res;
    success = executor.solver->mustBeTrue(state.constraints.cs(),
                                          EqExpr::create(arguments[1], value),
                                          res, state.queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");
    if (res) {
      llvm::errs() << " == " << value;
    } else {
      llvm::errs() << " ~= " << value;
      std::pair<ref<Expr>, ref<Expr>> res = executor.solver->getRange(
          state.constraints.cs(), arguments[1], state.queryMetaData);
      llvm::errs() << " (in [" << res.first << ", " << res.second << "])";
    }
  }
  llvm::errs() << "\n";
}

void SpecialFunctionHandler::handleGetObjSize(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_get_obj_size");
  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0],
                        executor.typeSystemManager->getUnknownType(), rl,
                        "klee_get_obj_size");
  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    const MemoryObject *mo =
        it->second->addressSpace.findObject(it->first).first;
    executor.bindLocal(
        target, *it->second,
        ConstantExpr::create(mo->size,
                             executor.kmodule->targetData->getTypeSizeInBits(
                                 target->inst->getType())));
  }
}

void SpecialFunctionHandler::handleGetErrno(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 0 &&
         "invalid number of arguments to klee_get_errno");
#ifndef WINDOWS
  int *errno_addr = executor.getErrnoLocation(state);
#else
  int *errno_addr = nullptr;
#endif

  // Retrieve the memory object of the errno variable
  IDType idErrnoObject;
  llvm::Type *pointerErrnoAddr = llvm::PointerType::get(
      llvm::IntegerType::get(executor.kmodule->module->getContext(),
                             sizeof(*errno_addr) * CHAR_BIT),
      executor.kmodule->targetData->getAllocaAddrSpace());

  bool resolved = state.addressSpace.resolveOne(
      ConstantExpr::createPointer((uint64_t)errno_addr),
      executor.typeSystemManager->getWrappedType(pointerErrnoAddr),
      idErrnoObject);
  if (!resolved)
    executor.terminateStateOnUserError(state,
                                       "Could not resolve address for errno");
  const ObjectState *os = state.addressSpace.findObject(idErrnoObject).second;
  executor.bindLocal(target, state, os->read(0, Expr::Int32));
}

void SpecialFunctionHandler::handleErrnoLocation(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  // Returns the address of the errno variable
  assert(arguments.size() == 0 &&
         "invalid number of arguments to __errno_location/__error");

#ifndef WINDOWS
  int *errno_addr = executor.getErrnoLocation(state);
#else
  int *errno_addr = nullptr;
#endif

  executor.bindLocal(
      target, state,
      ConstantExpr::create((uint64_t)errno_addr,
                           executor.kmodule->targetData->getTypeSizeInBits(
                               target->inst->getType())));
}
void SpecialFunctionHandler::handleCalloc(ExecutionState &state,
                                          KInstruction *target,
                                          std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 && "invalid number of arguments to calloc");

  ref<Expr> size = MulExpr::create(arguments[0], arguments[1]);
  executor.executeAlloc(state, size, false, target,
                        executor.typeSystemManager->handleAlloc(size), true,
                        nullptr, 0, CheckOutOfMemory);
}

void SpecialFunctionHandler::handleRealloc(ExecutionState &state,
                                           KInstruction *target,
                                           std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 2 && "invalid number of arguments to realloc");
  ref<Expr> address = arguments[0];
  ref<Expr> size = arguments[1];

  Executor::StatePair zeroSize = executor.forkInternal(
      state, Expr::createIsZero(size), BranchType::Realloc);

  if (zeroSize.first) { // size == 0
    executor.executeFree(*zeroSize.first, address, target);
  }
  if (zeroSize.second) { // size != 0
    Executor::StatePair zeroPointer = executor.forkInternal(
        *zeroSize.second, Expr::createIsZero(address), BranchType::Realloc);

    if (zeroPointer.first) { // address == 0
      executor.executeAlloc(*zeroPointer.first, size, false, target,
                            executor.typeSystemManager->handleAlloc(size),
                            false, nullptr, 0, CheckOutOfMemory);
    }
    if (zeroPointer.second) { // address != 0
      Executor::ExactResolutionList rl;
      executor.resolveExact(*zeroPointer.second, address,
                            executor.typeSystemManager->getUnknownType(), rl,
                            "realloc");

      for (Executor::ExactResolutionList::iterator it = rl.begin(),
                                                   ie = rl.end();
           it != ie; ++it) {
        const ObjectState *os =
            it->second->addressSpace.findObject(it->first).second;
        executor.executeAlloc(*it->second, size, false, target,
                              executor.typeSystemManager->handleRealloc(
                                  os->getDynamicType(), size),
                              false, os, 0, CheckOutOfMemory);
      }
    }
  }
}

void SpecialFunctionHandler::handleFree(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  // XXX should type check args
  assert(arguments.size() == 1 && "invalid number of arguments to free");
  executor.executeFree(state, arguments[0]);
}

void SpecialFunctionHandler::handleCheckMemoryAccess(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_check_memory_access");

  ref<Expr> address = executor.toUnique(state, arguments[0]);
  ref<Expr> size = executor.toUnique(state, arguments[1]);
  if (!isa<ConstantExpr>(address) || !isa<ConstantExpr>(size)) {
    executor.terminateStateOnUserError(
        state, "check_memory_access requires constant args");
  } else {
    IDType idObject;

    if (!state.addressSpace.resolveOne(
            cast<ConstantExpr>(address),
            executor.typeSystemManager->getUnknownType(), idObject)) {
      executor.terminateStateOnError(state, "check_memory_access: memory error",
                                     StateTerminationType::Ptr,
                                     executor.getAddressInfo(state, address));
    } else {
      const MemoryObject *mo = state.addressSpace.findObject(idObject).first;
      ref<Expr> chk = mo->getBoundsCheckPointer(
          address, cast<ConstantExpr>(size)->getZExtValue());
      if (!chk->isTrue()) {
        executor.terminateStateOnError(
            state, "check_memory_access: memory error",
            StateTerminationType::Ptr, executor.getAddressInfo(state, address));
      }
    }
  }
}

void SpecialFunctionHandler::handleGetValue(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_get_value");

  executor.executeGetValue(state, arguments[0], target);
}

void SpecialFunctionHandler::handleDefineFixedObject(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 2 &&
         "invalid number of arguments to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[0]) &&
         "expect constant address argument to klee_define_fixed_object");
  assert(isa<ConstantExpr>(arguments[1]) &&
         "expect constant size argument to klee_define_fixed_object");

  uint64_t address = cast<ConstantExpr>(arguments[0])->getZExtValue();
  uint64_t size = cast<ConstantExpr>(arguments[1])->getZExtValue();
  MemoryObject *mo =
      executor.memory->allocateFixed(address, size, state.prevPC->inst);
  executor.bindObjectInState(
      state, mo, executor.typeSystemManager->getUnknownType(), false);
  mo->isUserSpecified = true; // XXX hack;
}

void SpecialFunctionHandler::handleMakeSymbolic(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  std::string name;

  if (arguments.size() != 3) {
    executor.terminateStateOnUserError(
        state, "Incorrect number of arguments to klee_make_symbolic(void*, "
               "size_t, char*)");
    return;
  }

  name = arguments[2]->isZero() ? "" : readStringAtAddress(state, arguments[2]);

  if (name.length() == 0) {
    name = "unnamed";
    klee_warning("klee_make_symbolic: renamed empty name to \"unnamed\"");
  }

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0],
                        executor.typeSystemManager->getUnknownType(), rl,
                        "make_symbolic");

  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    ObjectPair op = it->second->addressSpace.findObject(it->first);
    const MemoryObject *mo = op.first;
    mo->setName(name);
    mo->updateTimestamp();

    const ObjectState *old = op.second;
    ExecutionState *s = it->second;

    if (old->readOnly) {
      executor.terminateStateOnUserError(
          *s, "cannot make readonly object symbolic");
      return;
    }

    // FIXME: Type coercion should be done consistently somewhere.
    bool res;
    bool success __attribute__((unused)) = executor.solver->mustBeTrue(
        s->constraints.cs(),
        EqExpr::create(
            ZExtExpr::create(arguments[1], Context::get().getPointerWidth()),
            mo->getSizeExpr()),
        res, s->queryMetaData);
    assert(success && "FIXME: Unhandled solver failure");

    if (res) {
      uint64_t sid = 0;
      if (state.arrayNames.count(name)) {
        sid = state.arrayNames[name];
      }
      executor.executeMakeSymbolic(
          *s, mo, old->getDynamicType(),
          SourceBuilder::makeSymbolic(name,
                                      executor.updateNameVersion(*s, name)),
          false);
    } else {
      executor.terminateStateOnUserError(
          *s, "Wrong size given to klee_make_symbolic");
    }
  }
}

void SpecialFunctionHandler::handleMarkGlobal(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to klee_mark_global");

  Executor::ExactResolutionList rl;
  executor.resolveExact(state, arguments[0],
                        executor.typeSystemManager->getUnknownType(), rl,
                        "mark_global");

  for (Executor::ExactResolutionList::iterator it = rl.begin(), ie = rl.end();
       it != ie; ++it) {
    const MemoryObject *mo =
        it->second->addressSpace.findObject(it->first).first;
    assert(!mo->isLocal);
    mo->isGlobal = true;
  }
}

template <typename... Ts>
inline std::string formattedMessage(const char *Fmt, const Ts &...Vals) {
  std::string message;
  llvm::raw_string_ostream ostream{message};
  ostream << llvm::format(Fmt, Vals...);
  return message;
}

void SpecialFunctionHandler::handleTypeMismatchV1(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  auto pointerExpr = arguments[1].get();
  auto pointer =
      executor.toConstant(state, pointerExpr, "handleTypeMismatchV1");
  if (pointer->isZero()) {
    executor.terminateStateOnError(state, "invalid usage of null pointer",
                                   StateTerminationType::UndefinedBehavior);
    return;
  } else {
    auto pointerAddress = pointer->getAPValue().getZExtValue();
    std::string message = formattedMessage(
        "either misaligned address for %p or invalid usage of address %p with "
        "insufficient space",
        pointerAddress, pointerAddress);
    executor.terminateStateOnError(state, message,
                                   StateTerminationType::UndefinedBehavior);
  }
}

void SpecialFunctionHandler::handleAlignmentAssumption(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  auto alignmentExpr = arguments[2].get();
  auto alignment =
      executor.toConstant(state, alignmentExpr, "handleAlignmentAssumption");
  auto alignmentValue = alignment->getZExtValue();
  auto offsetExpr = arguments[3].get();
  auto offset =
      executor.toConstant(state, offsetExpr, "handleAlignmentAssumption");
  auto offsetValue = offset->getZExtValue();
  if (offset->isZero()) {
    auto message = formattedMessage("assumption of %llu byte alignment failed",
                                    alignmentValue);
    executor.terminateStateOnError(state, message,
                                   StateTerminationType::UndefinedBehavior);
  } else {
    auto message = formattedMessage("assumption of %llu byte alignment (with "
                                    "offset of %llu byte) for pointer "
                                    "failed",
                                    alignmentValue, offsetValue);
    executor.terminateStateOnError(state, message,
                                   StateTerminationType::UndefinedBehavior);
  }
}

void SpecialFunctionHandler::handleAddOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on addition",
                                 StateTerminationType::Overflow);
}

void SpecialFunctionHandler::handleSubOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on subtraction",
                                 StateTerminationType::Overflow);
}

void SpecialFunctionHandler::handleMulOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on multiplication",
                                 StateTerminationType::Overflow);
}

void SpecialFunctionHandler::handleNegateOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "negation cannot be represented",
                                 StateTerminationType::Overflow);
}

void SpecialFunctionHandler::handleDivRemOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "overflow on division or remainder",
                                 StateTerminationType::Overflow);
}

void SpecialFunctionHandler::handleShiftOutOfBounds(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "shifted value is invalid",
                                 StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleOutOfBounds(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "index out of bounds",
                                 StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleBuiltinUnreachable(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state, "execution reached an unreachable program point",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleMissingReturn(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state,
      "execution reached the end of a value-returning function "
      "without returning a value",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleVlaBoundNotPositive(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state,
                                 "variable length array bound evaluates to "
                                 "non-positive value",
                                 StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleFloatCastOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state,
      "floating point value is outside the range of representable values",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleLoadInvalidValue(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "load invalid value",
                                 StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleImplicitConversion(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(state, "invalid implicit conversion",
                                 StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleInvalidBuiltin(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state,
      "passing zero to either ctz() or clz(), which is not a valid argument",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleNonnullReturnV1(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state,
      "null pointer returned from function declared to never return null",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleNullabilityReturnV1(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state,
      "null pointer returned from function declared to never return null",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleNonnullArg(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state,
      "null pointer passed as argument, which is declared to never be null",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handleNullabilityArg(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  executor.terminateStateOnError(
      state,
      "null pointer passed as argument, which is declared to never be null",
      StateTerminationType::UndefinedBehavior);
}

void SpecialFunctionHandler::handlePointerOverflow(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  auto baseExpr = arguments[1].get();
  auto base = executor.toConstant(state, baseExpr, "handlePointerOverflow");
  auto baseValue = base->getZExtValue();
  auto resultExpr = arguments[2].get();
  auto result = executor.toConstant(state, resultExpr, "handlePointerOverflow");
  auto resultValue = result->getZExtValue();
  if (base->isZero() && result->isZero()) {
    executor.terminateStateOnError(state,
                                   "applying zero offset to null pointer",
                                   StateTerminationType::UndefinedBehavior);
  } else if (base->isZero() && !result->isZero()) {
    auto message = formattedMessage(
        "applying non-zero offset %llu to null pointer", resultValue);
    executor.terminateStateOnError(state, message,
                                   StateTerminationType::UndefinedBehavior);
  } else if (!base->isZero() && result->isZero()) {
    auto message = formattedMessage(
        "applying non-zero offset to non-null pointer %p produced null pointer",
        baseValue);
    executor.terminateStateOnError(state, message,
                                   StateTerminationType::UndefinedBehavior);
  } else {
    auto message =
        formattedMessage("pointer arithmetic with base %p overflowed to %p",
                         baseValue, resultValue);
    executor.terminateStateOnError(state, message,
                                   StateTerminationType::UndefinedBehavior);
  }
}

void SpecialFunctionHandler::handleIsNaN(ExecutionState &state,
                                         KInstruction *target,
                                         std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsNaN");
  ref<Expr> result = IsNaNExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleIsInfinite(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsInfinite");
  ref<Expr> result = IsInfiniteExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleIsNormal(ExecutionState &state,
                                            KInstruction *target,
                                            std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsNormal");
  ref<Expr> result = IsNormalExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleIsSubnormal(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to IsSubnormal");
  ref<Expr> result = IsSubnormalExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleGetRoundingMode(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 0 &&
         "invalid number of arguments to GetRoundingMode");
  unsigned returnValue = 0;
  switch (state.roundingMode) {
  case llvm::APFloat::rmNearestTiesToEven:
    returnValue = KLEE_FP_RNE;
    break;
  case llvm::APFloat::rmNearestTiesToAway:
    returnValue = KLEE_FP_RNA;
    break;
  case llvm::APFloat::rmTowardPositive:
    returnValue = KLEE_FP_RU;
    break;
  case llvm::APFloat::rmTowardNegative:
    returnValue = KLEE_FP_RD;
    break;
  case llvm::APFloat::rmTowardZero:
    returnValue = KLEE_FP_RZ;
    break;
  default:
    klee_warning_once(nullptr, "Unknown llvm::APFloat rounding mode");
    returnValue = KLEE_FP_UNKNOWN;
  }
  // FIXME: The width is fragile. It's dependent on what the compiler
  // choose to be the width of the enum.
  ref<Expr> result = ConstantExpr::create(returnValue, Expr::Int32);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleSetConcreteRoundingMode(
    ExecutionState &state, KInstruction *target,
    std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 &&
         "invalid number of arguments to SetRoundingMode");
  llvm::APFloat::roundingMode newRoundingMode =
      llvm::APFloat::rmNearestTiesToEven;
  ref<Expr> roundingModeArg = arguments[0];
  if (!isa<ConstantExpr>(roundingModeArg)) {
    executor.terminateStateOnError(state, "argument should be concrete",
                                   StateTerminationType::User);
    return;
  }
  const ConstantExpr *CE = dyn_cast<ConstantExpr>(roundingModeArg);
  switch (CE->getZExtValue()) {
  case KLEE_FP_RNE:
    newRoundingMode = llvm::APFloat::rmNearestTiesToEven;
    break;
  case KLEE_FP_RNA:
    newRoundingMode = llvm::APFloat::rmNearestTiesToAway;
    break;
  case KLEE_FP_RU:
    newRoundingMode = llvm::APFloat::rmTowardPositive;
    break;
  case KLEE_FP_RD:
    newRoundingMode = llvm::APFloat::rmTowardNegative;
    break;
  case KLEE_FP_RZ:
    newRoundingMode = llvm::APFloat::rmTowardZero;
    break;
  default:
    executor.terminateStateOnError(state, "Invalid rounding mode",
                                   StateTerminationType::User);
    return;
  }
  state.roundingMode = newRoundingMode;
}

void SpecialFunctionHandler::handleSqrt(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to sqrt");
  ref<Expr> result = FSqrtExpr::create(arguments[0], state.roundingMode);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleRint(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to rint");
  ref<Expr> result = FRintExpr::create(arguments[0], state.roundingMode);
  executor.bindLocal(target, state, result);
}

void SpecialFunctionHandler::handleFAbs(ExecutionState &state,
                                        KInstruction *target,
                                        std::vector<ref<Expr>> &arguments) {
  assert(arguments.size() == 1 && "invalid number of arguments to fabs");
  ref<Expr> result = FAbsExpr::create(arguments[0]);
  executor.bindLocal(target, state, result);
}
