#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)
include(CMakeParseArguments)

function(add_global_cxx_flag flag)
  CMAKE_PARSE_ARGUMENTS(add_global_cxx_flag "REQUIRED" "" "" ${ARGN})
  string(REPLACE "-" "_" SANITIZED_FLAG_NAME "${flag}")
  string(REPLACE "/" "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  string(REPLACE "=" "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  string(REPLACE " " "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  string(REPLACE "+" "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  unset(HAS_${SANITIZED_FLAG_NAME})
  CHECK_CXX_COMPILER_FLAG("${flag}" HAS_${SANITIZED_FLAG_NAME}_CXX)
  if (add_global_cxx_flag_REQUIRED AND NOT HAS_${SANITIZED_FLAG_NAME}_CXX)
    message(FATAL_ERROR "The flag \"${flag}\" is required but your C++ compiler doesn't support it")
  endif()
  if (HAS_${SANITIZED_FLAG_NAME}_CXX)
    message(STATUS "C++ compiler supports ${flag}")
    # NOTE: Have to be careful here as CMAKE_CXX_FLAGS is a string
    # and not a list.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
  else()
    message(STATUS "C++ compiler does not support ${flag}")
  endif()
endfunction()

function(add_global_c_flag flag)
  CMAKE_PARSE_ARGUMENTS(add_global_c_flag "REQUIRED" "" "" ${ARGN})
  string(REPLACE "-" "_" SANITIZED_FLAG_NAME "${flag}")
  string(REPLACE "/" "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  string(REPLACE "=" "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  string(REPLACE " " "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  string(REPLACE "+" "_" SANITIZED_FLAG_NAME "${SANITIZED_FLAG_NAME}")
  unset(HAS_${SANITIZED_FLAG_NAME})
  CHECK_C_COMPILER_FLAG("${flag}" HAS_${SANITIZED_FLAG_NAME}_C)
  if (add_global_c_flag_REQUIRED AND NOT HAS_${SANITIZED_FLAG_NAME}_C)
    message(FATAL_ERROR "The flag \"${flag}\" is required but your C compiler doesn't support it")
  endif()
  if (HAS_${SANITIZED_FLAG_NAME}_C)
    message(STATUS "C compiler supports ${flag}")
    # NOTE: Have to be careful here as CMAKE_C_FLAGS is a string
    # and not a list.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
  else()
    message(STATUS "C compiler does not support ${flag}")
  endif()
endfunction()
