#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
if (UTIL_INCLUDE_DIR)
  add_executable(klee-replay
    fd_init.c
    file-creator.c
    klee-replay.c
    klee_init_env.c
  )

  target_link_libraries(klee-replay PRIVATE kleeBasic)
  target_include_directories(klee-replay PRIVATE ${KLEE_INCLUDE_DIRS})

  if(LIBCAP_LIBRARIES)
    target_link_libraries(klee-replay PRIVATE ${LIBCAP_LIBRARIES})
  endif()

  if (UTIL_LIBRARY)
    target_link_libraries(klee-replay PRIVATE ${UTIL_LIBRARY})
  endif (UTIL_LIBRARY)

  install(TARGETS klee-replay RUNTIME DESTINATION bin)
else()
  message(WARNING "Not building klee-replay due to missing library for pty functions.")
endif()
