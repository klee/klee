#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

# Top level target for building all documentation
add_custom_target(docs)

option(ENABLE_DOXYGEN "Enable building doxygen documentation" ON)
if (ENABLE_DOXYGEN)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    message(STATUS "Doxygen found")
    set(abs_top_srcdir "${CMAKE_SOURCE_DIR}")
    set(abs_top_builddir "${CMAKE_BINARY_DIR}")

    # Configure the Doxyfile
    configure_file(doxygen.cfg.in doxygen.cfg @ONLY)

    # Add rule to build doxygen documentation
    add_custom_target(doc-doxygen
      COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg"
      COMMENT "Generating Doxygen documentation"
      ${ADD_CUSTOM_COMMAND_USES_TERMINAL_ARG}
    )
    add_dependencies(docs doc-doxygen)

    # FIXME: This variable should be used to set `OUTPUT_DIRECTORY` in
    # doxygen.cfg
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

    # Add rule to clean doxygen documentation
    add_custom_target(clean_doxygen
      COMMAND
        "${CMAKE_COMMAND}"
        "-E"
        "remove_directory"
        "${DOXYGEN_OUTPUT_DIR}"
    )
    add_dependencies(clean_all clean_doxygen)
  else()
    message(WARNING "Doxygen not found. Can't build Doxygen documentation")
    set(ENABLE_DOXYGEN OFF
      CACHE
      BOOL
      "Enable building doxygen documentation" FORCE)
  endif()
endif()
