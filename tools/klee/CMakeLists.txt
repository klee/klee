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

# FIXME: This shouldn't be done here. Instead the KLEE libraries
# should declare their LLVM dependencies.
set(LLVM_COMPONENTS
	bitreader
	bitwriter
	ipo
	linker
	support
)

if ("${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "3.6" OR
    "${LLVM_PACKAGE_VERSION}" VERSION_GREATER "3.6")
  list(APPEND LLVM_COMPONENTS mcjit executionengine native)
else()
  list(APPEND LLVM_COMPONENTS jit engine)
endif()

if ("${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "3.3" OR
    "${LLVM_PACKAGE_VERSION}" VERSION_GREATER "3.3")
  list(APPEND LLVM_COMPONENTS irreader)
endif()

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})

set(KLEE_LIBS kleeCore kleeBasic kleeModule kleaverSolver kleaverExpr kleeSupport)

target_link_libraries(klee ${KLEE_LIBS} ${LLVM_LIBS})

install(TARGETS klee RUNTIME DESTINATION bin)

# The KLEE binary depends on the runtimes
add_dependencies(klee BuildKLEERuntimes)
