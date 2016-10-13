add_executable(gen-random-bout
	gen-random-bout.cpp
)

set(KLEE_LIBS kleeBasic)

target_link_libraries(gen-random-bout ${KLEE_LIBS})

install(TARGETS gen-random-bout RUNTIME DESTINATION bin)
