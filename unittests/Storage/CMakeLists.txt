add_klee_unit_test(StorageTest
  StorageTest.cpp)
target_link_libraries(StorageTest PRIVATE kleaverExpr kleaverSolver)
target_compile_options(StorageTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(StorageTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(StorageTest PRIVATE ${KLEE_INCLUDE_DIRS})
