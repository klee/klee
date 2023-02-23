#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
include(CMakePushCheckState)

find_package(Z3)
# Set the default so that if the following is true:
# * Z3 was found
# * ENABLE_SOLVER_Z3 is not already set as a cache variable
#
# then the default is set to `ON`. Otherwise set the default to `OFF`.
# A consequence of this is if we fail to detect Z3 the first time
# subsequent calls to CMake will not change the default.
if (Z3_FOUND)
  set(ENABLE_SOLVER_Z3_DEFAULT ON)
else()
  set(ENABLE_SOLVER_Z3_DEFAULT OFF)
endif()
option(ENABLE_SOLVER_Z3 "Enable Z3 solver support" ${ENABLE_SOLVER_Z3_DEFAULT})

if (ENABLE_SOLVER_Z3)
  message(STATUS "Z3 solver support enabled")
  if (Z3_FOUND)
    include(CheckCXXSourceCompiles)

    message(STATUS "Found Z3")
    set(ENABLE_Z3 1) # For config.h
    list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${Z3_INCLUDE_DIRS})
    list(APPEND KLEE_SOLVER_LIBRARIES ${Z3_LIBRARIES})

    # Check the signature of `Z3_get_error_msg()`
    cmake_push_check_state()
    set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${Z3_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${Z3_LIBRARIES})

    check_cxx_source_compiles("
    #include <z3.h>
    void custom_z3_error_handler(Z3_context ctx, Z3_error_code ec) {
      ::Z3_string errorMsg = Z3_get_error_msg(ctx, ec);
    }
    int main(int argc, char** argv) {
        return 0;
    }
    " HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
    cmake_pop_check_state()
    if (HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
      message(STATUS "Z3_get_error_msg requires context")
    else()
      message(STATUS "Z3_get_error_msg does not require context")
    endif()

    list(APPEND KLEE_SOLVER_INCLUDE_DIRS ${Z3_INCLUDE_DIRS})
    list(APPEND KLEE_SOLVER_LIBRARY_DIRS ${Z3_LIBRARIES})

  else()
    message(FATAL_ERROR "Z3 not found.")
  endif()
else()
  message(STATUS "Z3 solver support disabled")
  set(ENABLE_Z3 0) # For config.h
endif()
