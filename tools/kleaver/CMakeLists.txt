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

klee_get_llvm_libs(LLVM_LIBS support)
set(KLEE_LIBS kleeBasic kleaverSolver kleaverExpr kleeSupport)

target_link_libraries(kleaver ${KLEE_LIBS} ${LLVM_LIBS})

install(TARGETS kleaver RUNTIME DESTINATION bin)
