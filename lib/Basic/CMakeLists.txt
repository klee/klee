#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(kleeBasic
  KTest.cpp
  Statistics.cpp
)

llvm_config(kleeBasic "${USE_LLVM_SHARED}" support)
target_compile_options(kleeBasic PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeBasic PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(kleeBasic PRIVATE ${KLEE_INCLUDE_DIRS})
