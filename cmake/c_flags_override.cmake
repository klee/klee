#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
#
# This file overrides the default compiler flags for CMake's built-in
# configurations (CMAKE_BUILD_TYPE). Most compiler flags should not be set
# here.  The main purpose is to make sure ``-DNDEBUG`` is never set by default.
#
#===------------------------------------------------------------------------===#
if (("${CMAKE_C_COMPILER_ID}" MATCHES "Clang") OR ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU"))
  # Taken from Modules/Compiler/GNU.cmake but -DNDEBUG is removed
  set(CMAKE_C_FLAGS_INIT "")
  set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g")
  set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os")
  set(CMAKE_C_FLAGS_RELEASE_INIT "-O3")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g")
else()
  message(FATAL_ERROR "Overrides not set for compiler ${CMAKE_C_COMPILER_ID}")
endif()
