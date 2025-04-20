#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(kleeCore
  AddressSpace.cpp
  MergeHandler.cpp
  CallPathManager.cpp
  Context.cpp
  CoreStats.cpp
  ExecutionState.cpp
  ExecutionTree.cpp
  ExecutionTreeWriter.cpp
  Executor.cpp
  ExecutorUtil.cpp
  ExternalDispatcher.cpp
  ImpliedValue.cpp
  Memory.cpp
  MemoryManager.cpp
  Searcher.cpp
  SeedInfo.cpp
  SpecialFunctionHandler.cpp
  StatsTracker.cpp
  TimingSolver.cpp
  UserSearcher.cpp
  RLInterface.cpp
  RLPathSearcher.cpp
)

target_link_libraries(kleeCore PRIVATE
  kleeBasic
  kleeModule
  kleaverSolver
  kleaverExpr
  kleeSupport
)

# 添加 libtorch 链接，如果启用了 RL 搜索器
if (ENABLE_RL_SEARCHER)
  target_link_libraries(kleeCore PRIVATE ${TORCH_LIBRARIES})
endif()

llvm_config(kleeCore "${USE_LLVM_SHARED}" core executionengine mcjit native support)
target_link_libraries(kleeCore PRIVATE ${SQLite3_LIBRARIES})
target_include_directories(kleeCore PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS})
target_compile_options(kleeCore PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeCore PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
