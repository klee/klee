#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

find_package (PkgConfig REQUIRED)
pkg_check_modules(BITWUZLA IMPORTED_TARGET bitwuzla)

# Set the default so that if the following is true:
# * Bitwuzla was found
# * ENABLE_SOLVER_BITWUZLA is not already set as a cache variable
#
# then the default is set to `ON`. Otherwise set the default to `OFF`.
# A consequence of this is if we fail to detect Bitwuzla the first time
# subsequent calls to CMake will not change the default.

if(BITWUZLA_FOUND)
  set(ENABLE_SOLVER_BITWUZLA_DEFAULT ON)
else()
  set(ENABLE_SOLVER_BITWUZLA_DEFAULT OFF)
endif()

option(ENABLE_SOLVER_BITWUZLA "Enable Bitwuzla solver support" ${ENABLE_SOLVER_BITWUZLA_DEFAULT})

if (ENABLE_SOLVER_BITWUZLA)
  message(STATUS "Bitwuzla solver support enabled")
  if (BITWUZLA_FOUND)
    message(STATUS "Found Bitwuzla")
    set(ENABLE_BITWUZLA 1) # For config.h

    list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${BITWUZLA_INCLUDE_DIRS})
    list(APPEND KLEE_SOLVER_LIBRARIES ${BITWUZLA_LINK_LIBRARIES})
    list(APPEND KLEE_SOLVER_INCLUDE_DIRS ${BITWUZLA_INCLUDE_DIRS})
    list(APPEND KLEE_SOLVER_LIBRARY_DIRS ${BITWUZLA_LINK_LIBRARIES})

  else()
    message(FATAL_ERROR "Bitwuzla not found.")
  endif()
else()
  message(STATUS "Bitwuzla solver support disabled")
  set(ENABLE_BITWUZLA 0) # For config.h
endif()
