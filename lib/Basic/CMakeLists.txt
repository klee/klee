#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeBasic
  CmdLineOptions.cpp
  ConstructSolverChain.cpp
  KTest.cpp
  Statistics.cpp
)
set(LLVM_COMPONENTS
  support
)

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeBasic PUBLIC ${LLVM_LIBS})

target_link_libraries(kleeBasic PRIVATE
  # FIXME: THIS IS STUPID.
  # `ConstructSolverChain.cpp` should be in
  # `kleaverSolver` not in in `kleeBasic`.
  # We are creating a circular dependency because
  # of this because `kleaverSolver` depends on `kleeBasic`.
  kleaverSolver
)
