#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeCore
  AddressSpace.cpp
  MergeHandler.cpp
  CallPathManager.cpp
  Context.cpp
  CoreStats.cpp
  ExecutionState.cpp
  Executor.cpp
  ExecutorUtil.cpp
  ExternalDispatcher.cpp
  ImpliedValue.cpp
  Memory.cpp
  MemoryManager.cpp
  PTree.cpp
  Searcher.cpp
  SeedInfo.cpp
  SpecialFunctionHandler.cpp
  StatsTracker.cpp
  TimingSolver.cpp
  UserSearcher.cpp
)

# TODO: Work out what the correct LLVM components are for
# kleeCore.
set(LLVM_COMPONENTS
  core
  executionengine
  mcjit
  native
  support
)

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeCore PUBLIC ${LLVM_LIBS} ${SQLITE3_LIBRARIES})
target_link_libraries(kleeCore PRIVATE
  kleeBasic
  kleeModule
  kleaverSolver
  kleaverExpr
  kleeSupport
)
