#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(ktest-gen
  ktest-gen.cpp
)

set(KLEE_LIBS kleeBasic)

target_link_libraries(ktest-gen ${KLEE_LIBS})
target_include_directories(ktest-gen PRIVATE ${KLEE_INCLUDE_DIRS})

install(TARGETS ktest-gen RUNTIME DESTINATION bin)
