add_klee_unit_test(marktest3
  racecond.cpp)
target_compile_definitions(marktest3 PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_compile_options(marktest3 PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_include_directories(marktest3 PRIVATE ${KLEE_INCLUDE_DIRS})