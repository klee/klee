#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(kleeCore
  AddressManager.cpp
  AddressSpace.cpp
  CallPathManager.cpp
  Context.cpp
  CoreStats.cpp
  CXXTypeSystem/CXXTypeManager.cpp
  DistanceCalculator.cpp
  ExecutionState.cpp
  Executor.cpp
  ExecutorUtil.cpp
  ExternalDispatcher.cpp
  ImpliedValue.cpp
  Memory.cpp
  MemoryManager.cpp
  PForest.cpp
  MockBuilder.cpp
  PTree.cpp
  Searcher.cpp
  SeedInfo.cpp
  SpecialFunctionHandler.cpp
  StatsTracker.cpp
  TargetCalculator.cpp
  TargetedExecutionReporter.cpp
  TargetedExecutionManager.cpp
  TargetManager.cpp
  TimingSolver.cpp
  TypeManager.cpp
  UserSearcher.cpp
)

target_link_libraries(kleeCore PRIVATE
  kleeBasic
  kleeModule
  kleaverSolver
  kleaverExpr
  kleeSupport
)

llvm_config(kleeCore "${USE_LLVM_SHARED}" core executionengine mcjit native support)
target_link_libraries(kleeCore PRIVATE ${SQLite3_LIBRARIES})
target_include_directories(kleeCore PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS})
target_compile_options(kleeCore PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeCore PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
