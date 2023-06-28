add_klee_unit_test(KDAllocTest
  allocate.cpp
  randomtest.cpp
  reuse.cpp
  rusage.cpp
  sample.cpp
  stacktest.cpp)
target_compile_definitions(KDAllocTest PRIVATE USE_GTEST_INSTEAD_OF_MAIN)
target_compile_definitions(KDAllocTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_compile_options(KDAllocTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_include_directories(KDAllocTest PRIVATE ${KLEE_INCLUDE_DIRS})