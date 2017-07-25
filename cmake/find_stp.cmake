#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
# STP: Use CMake facility to detect. The user can pass `-DSTP_DIR=` to force
# a particular directory.

# Although find_package() will set `STP_DIR` for us add it here so that
# is displayed in `ccmake` and `cmake-gui`.
set(STP_DIR "" CACHE PATH "Path to directory containing STPConfig.cmake")
find_package(STP CONFIG)

# Set the default so that if the following is true:
# * STP was found
# * ENABLE_SOLVER_STP is not already set as a cache variable
#
# then the default is set to `ON`. Otherwise set the default to `OFF`.
# A consequence of this is if we fail to detect STP the first time
# subsequent calls to CMake will not change the default.
if (STP_FOUND)
  set(ENABLE_SOLVER_STP_DEFAULT ON)
else()
  set(ENABLE_SOLVER_STP_DEFAULT OFF)
endif()
option(ENABLE_SOLVER_STP "Enable STP solver support" ${ENABLE_SOLVER_STP_DEFAULT})

if (ENABLE_SOLVER_STP)
  message(STATUS "STP solver support enabled")
  if (STP_FOUND)
    message(STATUS "Found STP version ${STP_VERSION}")
    # Try the STP shared library first
    if ("${STP_SHARED_LIBRARY}" STREQUAL "")
      # Try the static library instead
      if ("${STP_STATIC_LIBRARY}" STREQUAL "")
        message(FATAL_ERROR "Couldn't find STP shared or static library")
      endif()
      message(STATUS "Using STP static library")
      list(APPEND KLEE_SOLVER_LIBRARIES "${STP_STATIC_LIBRARY}")
    else()
      message(STATUS "Using STP shared library")
      list(APPEND KLEE_SOLVER_LIBRARIES "${STP_SHARED_LIBRARY}")
    endif()
    list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS "${STP_INCLUDE_DIRS}")
    message(STATUS "STP_DIR: ${STP_DIR}")
    set(ENABLE_STP 1) # For config.h
  else()
    message(FATAL_ERROR "STP not found. Try setting `-DSTP_DIR=/path` where"
      " `/path` is the directory containing `STPConfig.cmake`")
  endif()
else()
  message(STATUS "STP solver support disabled")
  set(ENABLE_STP 0) # For config.h
endif()
