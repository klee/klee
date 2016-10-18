add_executable(klee
	main.cpp
)

klee_llvm_libnames(LLVM_LIBS
	bitreader
	bitwriter
	engine
	ipo
	irreader
	jit
	linker
	support
)
set(KLEE_LIBS kleeCore kleeBasic kleeModule kleaverSolver kleaverExpr kleeSupport)

target_link_libraries(klee ${KLEE_LIBS} ${LLVM_LIBS} ${TARGET_LIBS})

install(TARGETS klee RUNTIME DESTINATION bin)
