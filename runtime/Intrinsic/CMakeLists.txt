find_program(CLANG_PATH clang)
find_program(LLVM_LINK_PATH llvm-link)

klee_build_llvm_library(kleeRuntimeIntrinsic
	klee_div_zero_check.c
	klee_int.c
	klee_overshift_check.c
	klee_range.c
	memcpy.c
	memmove.c
	mempcpy.c
	memset.c
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kleeRuntimeIntrinsic.bc
	DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/klee/runtime)
