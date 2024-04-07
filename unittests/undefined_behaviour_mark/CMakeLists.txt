add_klee_unit_test(marktest2
  undefined_behaviour.cpp)
target_compile_definitions(marktest2 PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_compile_options(marktest2 PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_include_directories(marktest2 PRIVATE ${KLEE_INCLUDE_DIRS})