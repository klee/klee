#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeBasic
  KTest.cpp
  Statistics.cpp
)
set(LLVM_COMPONENTS
  support
)

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeBasic PRIVATE ${LLVM_LIBS})

install(TARGETS kleeBasic
        EXPORT run_klee
        DESTINATION "${RUN_KLEE_LIB_DEST}"
        )
