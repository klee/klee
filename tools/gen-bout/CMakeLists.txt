#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(gen-bout
  gen-bout.cpp
)

set(KLEE_LIBS kleeBasic)

target_link_libraries(gen-bout ${KLEE_LIBS})

install(TARGETS gen-bout RUNTIME DESTINATION bin)
