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


#define TERMINATION_CLASSES                                                    \
  TCLASS(Exit, 1U)                                                             \
  TCLASS(Early, 2U)                                                            \
  TCLASS(SolverError, 3U)                                                      \
  TCLASS(ProgramError, 4U)                                                     \
  TCLASS(UserError, 5U)                                                        \
  TCLASS(ExecutionError, 6U)                                                   \
  TCLASS(EarlyAlgorithm, 7U)                                                   \
  TCLASS(EarlyUser, 8U)

///@brief Termination classes categorize termination types
enum class StateTerminationClass : std::uint8_t {
  /// \cond DO_NOT_DOCUMENT
  #define TCLASS(N,I) N = (I),
  TERMINATION_CLASSES
  /// \endcond
};


// (Name, ID, file suffix)
#define TERMINATION_TYPES                                                      \
  TTYPE(RUNNING, 0U, "")                                                       \
  TTYPE(Exit, 1U, "")                                                          \
  TTMARK(EXIT, 1U)                                                             \
  TTYPE(Interrupted, 10U, "early")                                             \
  TTYPE(MaxDepth, 11U, "early")                                                \
  TTYPE(OutOfMemory, 12U, "early")                                             \
  TTYPE(OutOfStackMemory, 13U, "early")                                        \
  TTMARK(EARLY, 13U)                                                           \
  TTYPE(Solver, 20U, "solver.err")                                             \
  TTMARK(SOLVERERR, 20U)                                                       \
  TTYPE(Abort, 30U, "abort.err")                                               \
  TTYPE(Assert, 31U, "assert.err")                                             \
  TTYPE(BadVectorAccess, 32U, "bad_vector_access.err")                         \
  TTYPE(Free, 33U, "free.err")                                                 \
  TTYPE(Model, 34U, "model.err")                                               \
  TTYPE(Overflow, 35U, "overflow.err")                                         \
  TTYPE(Ptr, 36U, "ptr.err")                                                   \
  TTYPE(ReadOnly, 37U, "read_only.err")                                        \
  TTYPE(ReportError, 38U, "report_error.err")                                  \
  TTYPE(InvalidBuiltin, 39U, "invalid_builtin_use.err")                        \
  TTYPE(ImplicitTruncation, 40U, "implicit_truncation.err")                    \
  TTYPE(ImplicitConversion, 41U, "implicit_conversion.err")                    \
  TTYPE(UnreachableCall, 42U, "unreachable_call.err")                          \
  TTYPE(MissingReturn, 43U, "missing_return.err")                              \
  TTYPE(InvalidLoad, 44U, "invalid_load.err")                                  \
  TTYPE(NullableAttribute, 45U, "nullable_attribute.err")                      \
  TTMARK(PROGERR, 45U)                                                         \
  TTYPE(User, 50U, "user.err")                                                 \
  TTMARK(USERERR, 50U)                                                         \
  TTYPE(Execution, 60U, "exec.err")                                            \
  TTYPE(External, 61U, "external.err")                                         \
  TTMARK(EXECERR, 61U)                                                         \
  TTYPE(Replay, 70U, "")                                                       \
  TTYPE(Merge, 71U, "")                                                        \
  TTMARK(EARLYALGORITHM, 71U)                                                  \
  TTYPE(SilentExit, 80U, "")                                                   \
  TTMARK(EARLYUSER, 80U)                                                       \
  TTMARK(END, 80U)


///@brief Reason an ExecutionState got terminated.
enum class StateTerminationType : std::uint8_t {
  /// \cond DO_NOT_DOCUMENT
  #define TTYPE(N,I,S) N = (I),
  #define TTMARK(N,I) N = (I),
  TERMINATION_TYPES
  /// \endcond
};


// reset definitions

#undef TCLASS
#undef TTYPE
#undef TTMARK
#define TCLASS(N,I)
#define TTYPE(N,I,S)
#define TTMARK(N,I)

#endif
