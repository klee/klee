add_klee_unit_test(SearcherTest
  SearcherTest.cpp)
target_link_libraries(SearcherTest PRIVATE kleeCore)
target_include_directories(SearcherTest BEFORE PRIVATE "${CMAKE_SOURCE_DIR}/lib")
target_compile_options(SearcherTest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(SearcherTest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(SearcherTest PRIVATE ${KLEE_INCLUDE_DIRS})