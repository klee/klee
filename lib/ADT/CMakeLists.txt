#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(kleeADT
  SparseStorage.cpp
)

llvm_config(kleeADT "${USE_LLVM_SHARED}" support)
target_include_directories(kleeADT PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_options(kleeADT PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeADT PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
