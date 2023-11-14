#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(kleaverSolver
  AlphaEquivalenceSolver.cpp
  AssignmentValidatingSolver.cpp
  BitwuzlaBuilder.cpp
  BitwuzlaHashConfig.cpp
  BitwuzlaSolver.cpp
  CachingSolver.cpp
  CexCachingSolver.cpp
  ConstantDivision.cpp
  ConstructSolverChain.cpp
  ConcretizingSolver.cpp
  CoreSolver.cpp
  DummySolver.cpp
  FastCexSolver.cpp
  IncompleteSolver.cpp
  IndependentSolver.cpp
  MetaSMTSolver.cpp
  KQueryLoggingSolver.cpp
  QueryLoggingSolver.cpp
  SMTLIBLoggingSolver.cpp
  Solver.cpp
  SolverCmdLine.cpp
  SolverImpl.cpp
  SolverUtil.cpp
  SolverStats.cpp
  STPBuilder.cpp
  STPSolver.cpp
  ValidatingSolver.cpp
  Z3Builder.cpp
  Z3BitvectorBuilder.cpp
  Z3CoreBuilder.cpp
  Z3HashConfig.cpp
  Z3Solver.cpp
)

llvm_config(kleaverSolver "${USE_LLVM_SHARED}" support)
target_link_libraries(kleaverSolver PRIVATE
  kleeBasic
  kleaverExpr
  kleeSupport
  kleeModule
  ${KLEE_SOLVER_LIBRARIES})
target_include_directories(kleaverSolver PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS} ${KLEE_SOLVER_INCLUDE_DIRS})
target_compile_options(kleaverSolver PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleaverSolver PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

install(TARGETS kleaverSolver
        EXPORT run_klee
        DESTINATION "${RUN_KLEE_LIB_DEST}"
        )
