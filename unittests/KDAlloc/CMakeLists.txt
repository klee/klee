add_klee_unit_test(KDAllocTest
  allocate.cpp
  randomtest.cpp
  reuse.cpp
  rusage.cpp
  sample.cpp
  stacktest.cpp)
target_compile_definitions(KDAllocTest PUBLIC "-DKDALLOC_CHECKED" "-DUSE_KDALLOC" "-DUSE_GTEST_INSTEAD_OF_MAIN")
target_compile_definitions(KDAllocTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
target_compile_options(KDAllocTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
