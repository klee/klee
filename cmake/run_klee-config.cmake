get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

set(DEFAULT_CMAKE_BUILD_TYPE RelWithDebInfo)
set(DEFAULT_CMAKE_DIR ${SELF_DIR}/${CMAKE_BUILD_TYPE})
set(FALLBACK_CMAKE_DIR ${SELF_DIR}/${DEFAULT_CMAKE_BUILD_TYPE})
if (EXISTS ${DEFAULT_CMAKE_DIR}/run_klee.cmake)
    include(${DEFAULT_CMAKE_DIR}/run_klee.cmake)
else ()
    message(WARNING
            "Library with appropriate configuration ${CMAKE_BUILD_TYPE} not found.
Setting default configuration ${DEFAULT_CMAKE_BUILD_TYPE}.
Please install klee with -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} if you need.")
    if (EXISTS ${FALLBACK_CMAKE_DIR}/run_klee.cmake)
        include(${FALLBACK_CMAKE_DIR}/run_klee.cmake)
    else ()
        message(FATAL_ERROR "Library with default configuration ${DEFAULT_CMAKE_BUILD_TYPE} also not found.
Please install klee with appropriate or default configuration.")
    endif ()
endif ()
