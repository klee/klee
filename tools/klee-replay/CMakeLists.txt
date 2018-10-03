#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
if (HAVE_PTY_H OR HAVE_UTIL_H)
  add_executable(klee-replay
    fd_init.c
    file-creator.c
    klee-replay.c
    klee_init_env.c
  )

  target_link_libraries(klee-replay PRIVATE kleeBasic ${LIBCAP_LIBRARIES})
  if (openpty_in_libutil)
    target_link_libraries(klee-replay PRIVATE ${LIBUTIL_LIBRARIES})
  endif (openpty_in_libutil)
  install(TARGETS klee-replay RUNTIME DESTINATION bin)
else()
  message(WARNING "Not building klee-replay due to missing library for pty functions.")
endif()
