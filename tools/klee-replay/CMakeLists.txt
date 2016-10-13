add_executable(klee-replay
	fd_init.c
	file-creator.c
	klee-replay.c
	klee_init_env.c
)

target_link_libraries(klee-replay kleeBasic util cap)

install(TARGETS klee-replay RUNTIME DESTINATION bin)
