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

if (USE_MAP)
	llvm_map_components_to_libnames(llvm_libs support)
else()
	set(llvm_libs LLVM)
endif()

target_link_libraries(kleeBasic PRIVATE ${llvm_libs})
target_compile_options(kleeBasic PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeBasic PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_include_directories(kleeBasic PRIVATE ${KLEE_INCLUDE_DIRS})
