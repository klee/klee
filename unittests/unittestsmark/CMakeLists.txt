add_klee_unit_test(marktest
  deadlock.cpp
  racecond.cpp
  undefined_behaviour.cpp)
target_compile_definitions(marktest PRIVATE USE_GTEST_INSTEAD_OF_MAIN)
target_compile_definitions(marktest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_compile_options(marktest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_include_directories(marktest PRIVATE ${KLEE_INCLUDE_DIRS})