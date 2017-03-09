#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

###############################################################################
# Compiler warnings
#
# NOTE: All these variables should be lists of flags and NOT a single string.
###############################################################################
# FIXME: -Wunused-parameter fires a lot so for now suppress it.
set(GCC_AND_CLANG_WARNINGS_CXX
  "-Wall"
  "-Wextra"
  "-Wno-unused-parameter")
set(GCC_AND_CLANG_WARNINGS_C
  "-Wall"
  "-Wextra"
  "-Wno-unused-parameter")
set(GCC_ONLY_WARNINGS_C "")
set(GCC_ONLY_WARNINGS_CXX "")
set(CLANG_ONLY_WARNINGS_C "")
set(CLANG_ONLY_WARNINGS_CXX "")

###############################################################################
# Check which warning flags are supported and use them globally
###############################################################################
set(CXX_WARNING_FLAGS_TO_CHECK "")
set(C_WARNING_FLAGS_TO_CHECK "")

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  list(APPEND CXX_WARNING_FLAGS_TO_CHECK ${GCC_AND_CLANG_WARNINGS_CXX})
  list(APPEND CXX_WARNING_FLAGS_TO_CHECK ${GCC_ONLY_WARNINGS_CXX})
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  list(APPEND CXX_WARNING_FLAGS_TO_CHECK ${GCC_AND_CLANG_WARNINGS_CXX})
  list(APPEND CXX_WARNING_FLAGS_TO_CHECK ${CLANG_ONLY_WARNINGS_CXX})
else()
  message(AUTHOR_WARNING "Unknown compiler")
endif()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
  list(APPEND C_WARNING_FLAGS_TO_CHECK ${GCC_AND_CLANG_WARNINGS_C})
  list(APPEND C_WARNING_FLAGS_TO_CHECK ${GCC_ONLY_WARNINGS_C})
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  list(APPEND C_WARNING_FLAGS_TO_CHECK ${GCC_AND_CLANG_WARNINGS_C})
  list(APPEND C_WARNING_FLAGS_TO_CHECK ${CLANG_ONLY_WARNINGS_C})
else()
  message(AUTHOR_WARNING "Unknown compiler")
endif()

# Loop through flags and use the ones which the compiler supports
foreach (flag ${CXX_WARNING_FLAGS_TO_CHECK})
  # Note `add_global_cxx_flag()` is used rather than
  # `klee_component_add_cxx_flag()` because warning
  # flags are typically useful for building everything.
  add_global_cxx_flag("${flag}")
endforeach()
foreach (flag ${C_WARNING_FLAGS_TO_CHECK})
  add_global_c_flag("${flag}")
endforeach()

###############################################################################
# Warnings as errors
###############################################################################
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if (WARNINGS_AS_ERRORS)
  if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU"))
    add_global_cxx_flag("-Werror" REQUIRED)
    add_global_c_flag("-Werror" REQUIRED)
  else()
    message(AUTHOR_WARNING "Unknown compiler")
  endif()
  message(STATUS "Treating compiler warnings as errors")
else()
  message(STATUS "Not treating compiler warnings as errors")
endif()
