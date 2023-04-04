add_klee_unit_test(RNGTest
  RNGTest.cpp)
target_link_libraries(RNGTest PRIVATE kleeSupport)
target_compile_options(RNGTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(RNGTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(RNGTest PRIVATE ${KLEE_INCLUDE_DIRS})
