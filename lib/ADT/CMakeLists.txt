klee_add_component(kleeADT
  TestCaseUtils.cpp
  TestCase.cpp
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
target_link_libraries(kleeADT PUBLIC ${LLVM_LIBS} ${SQLITE3_LIBRARIES})
target_link_libraries(kleeADT PRIVATE
  kleeBasic
  kleeModule
  kleaverSolver
  kleaverExpr
  kleeSupport
)
