add_klee_unit_test(DiscretePDFTest
  DiscretePDFTest.cpp)
# FIXME add the following line to link against libgtest.a
target_link_libraries(DiscretePDFTest PRIVATE kleaverSolver)
target_compile_options(DiscretePDFTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(DiscretePDFTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(DiscretePDFTest PRIVATE ${KLEE_INCLUDE_DIRS})
