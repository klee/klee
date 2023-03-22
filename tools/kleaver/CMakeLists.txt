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

llvm_config(kleaver "${USE_LLVM_SHARED}" core support)

target_link_libraries(kleaver PRIVATE kleaverSolver)
target_include_directories(kleaver PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
target_compile_options(kleaver PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleaver PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})

target_compile_definitions(kleaver PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})


install(TARGETS kleaver RUNTIME DESTINATION bin)
