#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

add_executable(klee-ptree main.cpp Tree.cpp DFSVisitor.cpp Printers.cpp)

target_compile_features(klee-ptree PRIVATE cxx_std_17)
target_include_directories(klee-ptree PRIVATE ${KLEE_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS})
target_link_libraries(klee-ptree PUBLIC ${SQLite3_LIBRARIES})

install(TARGETS klee-ptree DESTINATION bin)
