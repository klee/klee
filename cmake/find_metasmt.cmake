#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

find_package(metaSMT CONFIG)
if (NOT metaSMT_FOUND)
  message(FATAL_ERROR "metaSMT not found. Try setting `-DmetaSMT_DIR=/path` where"
    " `/path` is the directory containing `metaSMTConfig.cmake`")
endif()
message(STATUS "metaSMT_DIR: ${metaSMT_DIR}")
list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS
  "${metaSMT_INCLUDE_DIR}" ${metaSMT_INCLUDE_DIRS})
# FIXME: We should test linking
list(APPEND KLEE_SOLVER_LIBRARIES ${metaSMT_LIBRARIES})

# THIS IS HORRIBLE. The ${metaSMT_CXXFLAGS} variable
# is badly formed. It is a string but we can't just split
# on spaces because its contents looks like this
# "  -DmetaSMT_BOOLECTOR_1_API -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS".
# So handle defines specially
string_to_list("${metaSMT_CXXFLAGS}" _metaSMT_CXXFLAGS_broken_list)
list(LENGTH _metaSMT_CXXFLAGS_broken_list _metaSMT_CXXFLAGS_broken_list_length)
math(EXPR _metaSMT_CXXFLAGS_broken_list_last_index "${_metaSMT_CXXFLAGS_broken_list_length} -1")
set(_metasmt_flags "")
set(_metasmt_defines "")
set(_index_to_skip "")
foreach (index RANGE 0 ${_metaSMT_CXXFLAGS_broken_list_last_index})
  list(FIND _index_to_skip "${index}" _should_skip)
  if ("${_should_skip}" EQUAL "-1")
    list(GET _metaSMT_CXXFLAGS_broken_list ${index} _current_flag)
    if ("${_current_flag}" MATCHES "^-D")
      # This is a define
      if ("${_current_flag}" MATCHES "^-D$")
        # This is a bad define. We have just `-D` and the next item
        # is the actually definition.
        if ("${index}" EQUAL "${_metaSMT_CXXFLAGS_broken_list_last_index}")
          message(FATAL_ERROR "Stray -D flag!")
        else()
          # Get next value
          math(EXPR _next_index "${index} + 1")
          list(GET _metaSMT_CXXFLAGS_broken_list ${_next_index} _next_flag)
          if ("${_next_flag}" STREQUAL "")
            message(FATAL_ERROR "Next flag shouldn't be empty!")
          endif()
          list(APPEND _metasmt_defines "-D${_next_flag}")
          list(APPEND _index_to_skip "${_next_index}")
        endif()
      else()
        # This is well formed defined (e.g. `-DHELLO`)
        list(APPEND _metasmt_defines "${_current_flag}")
      endif()
    else()
      # Regular flag
      list(APPEND _metasmt_flags "${_current_flag}")
    endif()
  endif()
endforeach()

message(STATUS "metaSMT defines: ${_metasmt_defines}")
list(APPEND KLEE_COMPONENT_CXX_DEFINES ${_metasmt_defines})

message(STATUS "metaSMT flags: ${_metasmt_flags}")
foreach (f ${_metasmt_flags})
  # Test the flag and fail if it can't be used
  klee_component_add_cxx_flag(${f} REQUIRED)
endforeach()

set(available_metasmt_backends "BTOR" "STP" "Z3")
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

# Set appropriate define
list(APPEND KLEE_COMPONENT_CXX_DEFINES
  -DMETASMT_DEFAULT_BACKEND_IS_${METASMT_DEFAULT_BACKEND})
