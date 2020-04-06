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
      USES_TERMINAL
    )
    add_dependencies(docs doc-doxygen)

    # FIXME: This variable should be used to set `OUTPUT_DIRECTORY` in
    # doxygen.cfg
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

    if ("${CMAKE_VERSION}" VERSION_LESS "3.15")
      set_directory_properties(PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
              "${DOXYGEN_OUTPUT_DIR}")
    else ()
      set_directory_properties(PROPERTY ADDITIONAL_CLEAN_FILES
              "${DOXYGEN_OUTPUT_DIR}")
    endif ()

  else()
    message(WARNING "Doxygen not found. Can't build Doxygen documentation")
    set(ENABLE_DOXYGEN OFF
      CACHE
      BOOL
      "Enable building doxygen documentation" FORCE)
  endif()
endif()
