#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_executable(kleaver
  main.cpp
)

if (USE_MAP)
	llvm_map_components_to_libnames(llvm_libs core support)
else()
	set(llvm_libs LLVM)
endif()

target_link_libraries(kleaver kleaverSolver ${llvm_libs})
target_include_directories(kleaver PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_options(kleaver PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleaver PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_compile_definitions(kleaver PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})


install(TARGETS kleaver RUNTIME DESTINATION bin)
