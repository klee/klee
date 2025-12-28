#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

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
    message(STATUS "Found Z3")
    set(ENABLE_Z3 1) # For config.h

    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      string(REPLACE ".so" ".dylib" Z3_LIBRARIES ${Z3_LIBRARIES})
      message(STATUS "New Z3 library path: ${Z3_LIBRARIES}")

      if(EXISTS "${Z3_LIBRARIES}")
        message(STATUS "Found Z3 library on MacOS")
      else()
        message(STATUS "Z3 library not found on MacOS.")
      endif()
    endif()

    list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${Z3_INCLUDE_DIRS})
    list(APPEND KLEE_SOLVER_LIBRARIES ${Z3_LIBRARIES})
    list(APPEND KLEE_SOLVER_INCLUDE_DIRS ${Z3_INCLUDE_DIRS})

  else()
    message(FATAL_ERROR "Z3 not found.")
  endif()
else()
  message(STATUS "Z3 solver support disabled")
  set(ENABLE_Z3 0) # For config.h
endif()
