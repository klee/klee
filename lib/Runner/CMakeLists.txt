klee_add_component(kleeRunner
        parse_static_analysis_json.cpp
        run_klee.cpp
        )

set(KLEE_LIBS
        kleeCore
        )
set(RUN_KLEE_INCLUDE_DEST "include/run_klee")

target_include_directories(kleeRunner PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/klee/Runner>
        $<INSTALL_INTERFACE:${RUN_KLEE_INCLUDE_DEST}>
        $<INSTALL_INTERFACE:${RUN_KLEE_LIB_DEST}>
        )
target_link_libraries(kleeRunner PUBLIC ${KLEE_LIBS})

set(run_klee_header_path ${PROJECT_SOURCE_DIR}/include/klee/Runner)
set(run_klee_headers
        ${run_klee_header_path}/run_klee.h)

set_target_properties(kleeRunner PROPERTIES PUBLIC_HEADER "${run_klee_headers}")

install(TARGETS kleeRunner
        EXPORT run_klee DESTINATION "${RUN_KLEE_LIB_DEST}"
        PUBLIC_HEADER DESTINATION "${RUN_KLEE_INCLUDE_DEST}"
        )


# The KLEE depends on the runtimes
add_dependencies(kleeRunner BuildKLEERuntimes)
