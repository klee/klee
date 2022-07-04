//===-- TerminationTypes.h --------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_TERMINATIONTYPES_H
#define KLEE_TERMINATIONTYPES_H

#include <cstdint>

#define TERMINATION_TYPES                                                      \
  TTYPE(RUNNING, 0U, "")                                                       \
  TTYPE(Exit, 1U, "")                                                          \
  MARK(NORMAL, 1U)                                                             \
  TTYPE(Interrupted, 2U, "early")                                              \
  TTYPE(MaxDepth, 3U, "early")                                                 \
  TTYPE(OutOfMemory, 4U, "early")                                              \
  TTYPE(OutOfStackMemory, 5U, "early")                                         \
  MARK(EARLY, 5U)                                                              \
  TTYPE(Solver, 8U, "solver.err")                                              \
  MARK(SOLVERERR, 8U)                                                          \
  TTYPE(Abort, 10U, "abort.err")                                               \
  TTYPE(Assert, 11U, "assert.err")                                             \
  TTYPE(BadVectorAccess, 12U, "bad_vector_access.err")                         \
  TTYPE(Free, 13U, "free.err")                                                 \
  TTYPE(Model, 14U, "model.err")                                               \
  TTYPE(Overflow, 15U, "overflow.err")                                         \
  TTYPE(Ptr, 16U, "ptr.err")                                                   \
  TTYPE(ReadOnly, 17U, "read_only.err")                                        \
  TTYPE(ReportError, 18U, "report_error.err")                                  \
  TTYPE(InvalidBuiltin, 19U, "invalid_builtin_use.err")                        \
  TTYPE(ImplicitTruncation, 20U, "implicit_truncation.err")                    \
  TTYPE(ImplicitConversion, 21U, "implicit_conversion.err")                    \
  TTYPE(UnreachableCall, 22U, "unreachable_call.err")                          \
  TTYPE(MissingReturn, 23U, "missing_return.err")                              \
  TTYPE(InvalidLoad, 24U, "invalid_load.err")                                  \
  TTYPE(NullableAttribute, 25U, "nullable_attribute.err")                      \
  MARK(PROGERR, 25U)                                                           \
  TTYPE(User, 33U, "user.err")                                                 \
  MARK(USERERR, 33U)                                                           \
  TTYPE(Execution, 35U, "exec.err")                                            \
  TTYPE(External, 36U, "external.err")                                         \
  MARK(EXECERR, 36U)                                                           \
  TTYPE(Replay, 37U, "")                                                       \
  TTYPE(Merge, 38U, "")                                                        \
  TTYPE(SilentExit, 39U, "")                                                   \
  MARK(END, 39U)

///@brief Reason an ExecutionState got terminated.
enum class StateTerminationType : std::uint8_t {
#define TTYPE(N,I,S) N = (I),
#define MARK(N,I) N = (I),
  TERMINATION_TYPES
#undef TTYPE
#undef MARK
};

#endif
