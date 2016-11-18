#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(klee
  main.cpp
)

set(KLEE_LIBS
  kleeCore
)

target_link_libraries(klee ${KLEE_LIBS})

install(TARGETS klee RUNTIME DESTINATION bin)

# The KLEE binary depends on the runtimes
add_dependencies(klee BuildKLEERuntimes)
