add_executable(kleaver
	main.cpp
)

klee_llvm_libnames(LLVM_LIBS support)
set(KLEE_LIBS kleeBasic kleaverSolver kleaverExpr kleeSupport)

target_link_libraries(kleaver ${KLEE_LIBS} ${LLVM_LIBS} ${TARGET_LIBS})

install(TARGETS kleaver RUNTIME DESTINATION bin)
