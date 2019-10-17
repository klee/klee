add_klee_unit_test(SearcherTest
  SearcherTest.cpp)
target_link_libraries(SearcherTest PRIVATE kleeCore)
target_include_directories(SearcherTest BEFORE PUBLIC "../../lib")
