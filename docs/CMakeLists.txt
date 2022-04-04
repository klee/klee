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

option(ENABLE_DOXYGEN "Enable building doxygen documentation" OFF)
if (ENABLE_DOXYGEN)
  find_package(Doxygen OPTIONAL_COMPONENTS dot)
  if (DOXYGEN_FOUND AND TARGET Doxygen::dot)
    message(STATUS "Doxygen and dot found")

    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

    # Configure the Doxyfile
    configure_file(doxygen.cfg.in doxygen.cfg @ONLY)

    # Add rule to build doxygen documentation
    add_custom_target(doc-doxygen
      COMMAND Doxygen::doxygen "${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg"
      COMMENT "Generating Doxygen documentation"
      USES_TERMINAL
    )
    add_dependencies(docs doc-doxygen)

    set_directory_properties(PROPERTY ADDITIONAL_CLEAN_FILES
      "${DOXYGEN_OUTPUT_DIR}")

  else()
    if (NOT DOXYGEN_FOUND)
      message(WARNING "Doxygen not found. Can't build Doxygen documentation")
    endif ()
    if (NOT TARGET Doxygen::dot)
      message(WARNING "dot (graphviz) not found. Can't build Doxygen documentation")
    endif ()
    set(ENABLE_DOXYGEN OFF
      CACHE
      BOOL
      "Enable building doxygen documentation" FORCE)
  endif()
endif()
