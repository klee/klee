#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_subdirectory(Runtest)

if ("${KLEE_RUNTIME_BUILD_TYPE}" MATCHES "Release")
  set(RUNTIME_IS_RELEASE 1)
else()
  set(RUNTIME_IS_RELEASE 0)
endif()

if ("${KLEE_RUNTIME_BUILD_TYPE}" MATCHES "Asserts")
  set(RUNTIME_HAS_ASSERTIONS 1)
else()
  set(RUNTIME_HAS_ASSERTIONS 0)
endif()

if ("${KLEE_RUNTIME_BUILD_TYPE}" MATCHES "Debug")
  set(RUNTIME_HAS_DEBUG_SYMBOLS 1)
else()
  set(RUNTIME_HAS_DEBUG_SYMBOLS 0)
endif()


# FIXME: This is a horrible hack that needs to die.
# Things are very inconsistent. The runtime instrinsic
# is sometimes a LLVM module or a bitcode archive.
if ("${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "3.3" OR
    "${LLVM_PACKAGE_VERSION}" VERSION_GREATER "3.3")
  set(USE_RUNTIME_BINARY_TYPE_HACK 1)
else()
  set(USE_RUNTIME_BINARY_TYPE_HACK 0)
endif()

if (ENABLE_POSIX_RUNTIME)
  set(BUILD_POSIX_RUNTIME 1)
else()
  set(BUILD_POSIX_RUNTIME 0)
endif()

# Configure the bitcode build system
configure_file("Makefile.cmake.bitcode.config.in"
  "Makefile.cmake.bitcode.config"
  @ONLY
)

# Copy over the makefiles to the build directory
configure_file("Makefile.cmake.bitcode" "Makefile.cmake.bitcode" COPYONLY)
configure_file("Makefile.cmake.bitcode.rules" "Makefile.cmake.bitcode.rules" COPYONLY)

# Makefile for root runtime directory
# Copy over makefiles for libraries
set(BITCODE_LIBRARIES "Intrinsic" "klee-libc")
if (ENABLE_POSIX_RUNTIME)
  list(APPEND BITCODE_LIBRARIES "POSIX")
endif()
foreach (bl ${BITCODE_LIBRARIES})
  configure_file("${bl}/Makefile.cmake.bitcode"
    "${CMAKE_CURRENT_BINARY_DIR}/${bl}/Makefile.cmake.bitcode"
    COPYONLY)
endforeach()

# Find GNU make
find_program(MAKE_BINARY
  NAMES make gmake
)

if (NOT MAKE_BINARY)
  message(STATUS "Failed to find make binary")
endif()

# Find env
find_program(ENV_BINARY
  NAMES env
)
if (NOT ENV_BINARY)
  message(FATAL_ERROR "Failed to find env binary")
endif()

option(KLEE_RUNTIME_ALWAYS_REBUILD "Always try to rebuild KLEE runtime" ON)
if (KLEE_RUNTIME_ALWAYS_REBUILD)
  set(EXTERNAL_PROJECT_BUILD_ALWAYS_ARG 1)
else()
  set(EXTERNAL_PROJECT_BUILD_ALWAYS_ARG 0)
endif()

# Build the runtime as an external project.
# We do this because CMake isn't really suitable
# for building the runtime because it can't handle
# the source file dependencies properly.
include(ExternalProject)
ExternalProject_Add(BuildKLEERuntimes
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E echo "" # Dummy command
  BUILD_COMMAND "${CMAKE_COMMAND}" -E echo "" # Dummy command
  INSTALL_COMMAND "${CMAKE_COMMAND}" -E echo "" # Dummy command
)

# Use `ExternalProject_Add_Step` with `ALWAYS` argument instead of directly
# building in `ExternalProject_Add` with `BUILD_ALWAYS` argument due to lack of
# support for the `BUILD_ALWAYS` argument in CMake < 3.1.
ExternalProject_Add_Step(BuildKLEERuntimes RuntimeBuild
  # `env` is used here to make sure `MAKEFLAGS` of KLEE's build
  # is not propagated into the bitcode build system.
  COMMAND ${ENV_BINARY} MAKEFLAGS="" ${MAKE_BINARY} -f Makefile.cmake.bitcode all
  ALWAYS ${EXTERNAL_PROJECT_BUILD_ALWAYS_ARG}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  ${EXTERNAL_PROJECT_ADD_STEP_USES_TERMINAL_ARG}
)

# Target for cleaning the bitcode build system
# FIXME: Invoke `make clean` does not invoke this target. It's also weird
# that `ExternalProject` provides no way to do a clean.
add_custom_target(clean_runtime
  COMMAND ${ENV_BINARY} MAKEFLAGS="" ${MAKE_BINARY} -f Makefile.cmake.bitcode clean
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  ${ADD_CUSTOM_COMMAND_USES_TERMINAL_ARG}
)

###############################################################################
# Runtime install
###############################################################################
set(RUNTIME_FILES_TO_INSTALL)

# This is quite fragile and depends on knowledge in the bitcode
# build system. Hopefully it won't change very often though.

# FIXME: This hack needs to die!
if (USE_RUNTIME_BINARY_TYPE_HACK)
  list(APPEND RUNTIME_FILES_TO_INSTALL
    "${KLEE_RUNTIME_DIRECTORY}/kleeRuntimeIntrinsic.bc"
    "${KLEE_RUNTIME_DIRECTORY}/klee-libc.bc")
else()
  list(APPEND RUNTIME_FILES_TO_INSTALL
    "${KLEE_RUNTIME_DIRECTORY}/libkleeRuntimeIntrinsic.bca"
    "${KLEE_RUNTIME_DIRECTORY}/libklee-libc.bca")
endif()

if (ENABLE_POSIX_RUNTIME)
  list(APPEND RUNTIME_FILES_TO_INSTALL
    "${KLEE_RUNTIME_DIRECTORY}/libkleeRuntimePOSIX.bca")
endif()

install(FILES
  ${RUNTIME_FILES_TO_INSTALL}
  DESTINATION "${KLEE_INSTALL_RUNTIME_DIR}")
