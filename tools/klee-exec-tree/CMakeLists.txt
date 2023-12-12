#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

add_executable(klee-exec-tree main.cpp Tree.cpp DFSVisitor.cpp Printers.cpp)

target_compile_features(klee-exec-tree PRIVATE cxx_std_17)
target_include_directories(klee-exec-tree PRIVATE ${KLEE_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS})
target_link_libraries(klee-exec-tree PUBLIC ${SQLite3_LIBRARIES})

install(TARGETS klee-exec-tree DESTINATION bin)
