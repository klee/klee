# Tries to find an install of the Z3 library and header files
#
# Once done this will define
#  Z3_FOUND - BOOL: System has the Z3 library installed
#  Z3_INCLUDE_DIRS - LIST:The GMP include directories
#  Z3_LIBRARIES - LIST:The libraries needed to use Z3
include(FindPackageHandleStandardArgs)

# Try to find libraries
find_library(Z3_LIBRARIES
  NAMES z3
  DOC "Z3 libraries"
)
if (Z3_LIBRARIES)
  message(STATUS "Found Z3 libraries: \"${Z3_LIBRARIES}\"")
else()
  message(STATUS "Could not find Z3 libraries")
endif()

# Try to find headers
find_path(Z3_INCLUDE_DIRS
  NAMES z3.h
  # For distributions that keep the header files in a `z3` folder,
  # for example Fedora's `z3-devel` package at `/usr/include/z3/z3.h`
  PATH_SUFFIXES z3
  DOC "Z3 C header"
)
if (Z3_INCLUDE_DIRS)
  message(STATUS "Found Z3 include path: \"${Z3_INCLUDE_DIRS}\"")
else()
  message(STATUS "Could not find Z3 include path")
endif()

# TODO: We should check we can link some simple code against libz3

# Handle QUIET and REQUIRED and check the necessary variables were set and if so
# set ``Z3_FOUND``
find_package_handle_standard_args(Z3 DEFAULT_MSG Z3_INCLUDE_DIRS Z3_LIBRARIES)
