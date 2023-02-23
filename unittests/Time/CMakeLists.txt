add_klee_unit_test(TimeTest
  TimeTest.cpp)
target_link_libraries(TimeTest PRIVATE kleeSupport)
target_compile_options(TimeTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(TimeTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_include_directories(TimeTest PRIVATE ${KLEE_INCLUDE_DIRS})
