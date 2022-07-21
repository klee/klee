#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeCore
  AddressManager.cpp  
  AddressSpace.cpp
  MergeHandler.cpp
  CallPathManager.cpp
  Context.cpp
  CoreStats.cpp
  CXXTypeSystem/CXXTypeManager.cpp
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
  Target.cpp
  TimingSolver.cpp
  TypeManager.cpp
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
target_link_libraries(kleeCore PRIVATE ${LLVM_LIBS} ${SQLITE3_LIBRARIES})
target_link_libraries(kleeCore PUBLIC
  kleeBasic
  kleeModule
  kleaverSolver
  kleaverExpr
  kleeSupport
)

install(TARGETS kleeCore
        EXPORT run_klee
        DESTINATION "${RUN_KLEE_LIB_DEST}"
        )
