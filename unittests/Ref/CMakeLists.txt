add_klee_unit_test(RefTest
  RefTest.cpp)
target_link_libraries(RefTest PRIVATE kleaverExpr)
target_compile_options(RefTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(RefTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(RefTest PRIVATE ${KLEE_INCLUDE_DIRS})
