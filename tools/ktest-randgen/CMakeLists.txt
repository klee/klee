#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(ktest-randgen
  ktest-randgen.cpp
)

set(KLEE_LIBS kleeBasic)

target_link_libraries(ktest-randgen ${KLEE_LIBS})
target_include_directories(ktest-randgen PRIVATE ${KLEE_INCLUDE_DIRS})

install(TARGETS ktest-randgen RUNTIME DESTINATION bin)
