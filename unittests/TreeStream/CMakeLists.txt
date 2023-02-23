add_klee_unit_test(TreeStreamTest
  TreeStreamTest.cpp)
target_link_libraries(TreeStreamTest PRIVATE kleeBasic kleeSupport)
target_compile_options(TreeStreamTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(TreeStreamTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_include_directories(TreeStreamTest PRIVATE ${KLEE_INCLUDE_DIRS})
