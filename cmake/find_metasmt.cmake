#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
find_package(metaSMT QUIET CONFIG)

# Set the default so that if the following is true:
# * METASMT was found
# * ENABLE_SOLVER_METASMT is not already set as a cache variable
#
# then the default is set to `ON`. Otherwise set the default to `OFF`.
# A consequence of this is if we fail to detect METASMT the first time
# subsequent calls to CMake will not change the default.
if (metaSMT_FOUND)
  set(ENABLE_SOLVER_METASMT_DEFAULT ON)
else()
  set(ENABLE_SOLVER_METASMT_DEFAULT OFF)
endif()
option(ENABLE_SOLVER_METASMT
  "Enable metaSMT solver support"
  ${ENABLE_SOLVER_METASMT_DEFAULT}
)

if (ENABLE_SOLVER_METASMT)
  message(STATUS "metaSMT solver support enabled")
  set(ENABLE_METASMT 1) # For config.h

  if (NOT metaSMT_FOUND)
    message(FATAL_ERROR "metaSMT not found. Try setting `-DmetaSMT_DIR=/path` where"
      " `/path` is the directory containing `metaSMTConfig.cmake`")
  endif()
  message(STATUS "metaSMT_DIR: ${metaSMT_DIR}")
  list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS
    "${metaSMT_INCLUDE_DIR}" ${metaSMT_INCLUDE_DIRS})
  # FIXME: We should test linking
  list(APPEND KLEE_SOLVER_LIBRARIES ${metaSMT_LIBRARIES})

  # Separate flags and defines from ${metaSMT_CXXFLAGS}
  string_to_list("${metaSMT_CXXFLAGS}" _metaSMT_CXXFLAGS_list)
  set(_metasmt_flags "")
  set(_metasmt_defines "")
  foreach (flag ${_metaSMT_CXXFLAGS_list})
    if ("${flag}" MATCHES "^-D")
      # This is a define
      list(APPEND _metasmt_defines "${flag}")
    else()
      # Regular flag
      list(APPEND _metasmt_flags "${flag}")
    endif()
  endforeach()

  message(STATUS "metaSMT defines: ${_metasmt_defines}")
  list(APPEND KLEE_COMPONENT_CXX_DEFINES ${_metasmt_defines})

  message(STATUS "metaSMT flags: ${_metasmt_flags}")
  foreach (f ${_metasmt_flags})
    # Test the flag and fail if it can't be used
    klee_component_add_cxx_flag(${f} REQUIRED)
  endforeach()

  # Check if metaSMT provides an useable backend
  if (NOT metaSMT_AVAILABLE_QF_ABV_SOLVERS)
    message(FATAL_ERROR "metaSMT does not provide an useable backend.")
  endif()

  message(STATUS "metaSMT has the following backend(s): ${metaSMT_AVAILABLE_QF_ABV_SOLVERS}.")
  set(available_metasmt_backends ${metaSMT_AVAILABLE_QF_ABV_SOLVERS})
  set(METASMT_DEFAULT_BACKEND "STP"
    CACHE
    STRING
    "Default metaSMT backend. Availabe options ${available_metasmt_backends}")
  # Provide drop down menu options in cmake-gui
  set_property(CACHE METASMT_DEFAULT_BACKEND
    PROPERTY STRINGS ${available_metasmt_backends})

  # Check METASMT_DEFAULT_BACKEND has a valid value.
  list(FIND available_metasmt_backends "${METASMT_DEFAULT_BACKEND}" _meta_smt_backend_index)
  if ("${_meta_smt_backend_index}" EQUAL "-1")
    message(FATAL_ERROR
      "Invalid metaSMT default backend (\"${METASMT_DEFAULT_BACKEND}\").\n"
      "Valid values are ${available_metasmt_backends}")
  endif()

  # Set appropriate defines
  list(APPEND KLEE_COMPONENT_CXX_DEFINES
    -DMETASMT_DEFAULT_BACKEND_IS_${METASMT_DEFAULT_BACKEND})
  foreach(backend ${available_metasmt_backends})
    list(APPEND KLEE_COMPONENT_CXX_DEFINES -DMETASMT_HAVE_${backend})
  endforeach()
else()
  message(STATUS "metaSMT solver support disabled")
  set(ENABLE_METASMT 0) # For config.h
endif()
