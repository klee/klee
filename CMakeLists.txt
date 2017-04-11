#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

###############################################################################
# Minimum CMake version and policies
###############################################################################
cmake_minimum_required(VERSION 2.8.12)
if (POLICY CMP0054)
  # FIXME: This is horrible. With the old behaviour,
  # quoted strings like "MSVC" in if() conditionals
  # get implicitly dereferenced. The NEW behaviour
  # doesn't do this but CMP0054 was only introduced
  # in CMake 3.1 and we support lower versions as the
  # minimum. We could set NEW here but it would be very
  # confusing to use NEW for some builds and OLD for others
  # which could lead to some subtle bugs. Instead when the
  # minimum version is 3.1 change this policy to NEW and remove
  # the hacks in place to work around it.
  cmake_policy(SET CMP0054 OLD)
endif()

if (POLICY CMP0042)
  # Enable `MACOSX_RPATH` by default.
  cmake_policy(SET CMP0042 NEW)
endif()

if (POLICY CMP0037)
  # Disallow reserved target names
  cmake_policy(SET CMP0037 NEW)
endif()

# This overrides the default flags for the different CMAKE_BUILD_TYPEs
set(CMAKE_USER_MAKE_RULES_OVERRIDE_C
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flags_override.cmake")
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flags_override.cmake")
project(KLEE CXX C)

###############################################################################
# Project version
###############################################################################
set(KLEE_VERSION_MAJOR 1)
set(KLEE_VERSION_MINOR 3)
set(KLEE_VERSION_PATCH 0)
set(KLEE_VERSION_TWEAK 0)
set(KLEE_VERSION "${KLEE_VERSION_MAJOR}.${KLEE_VERSION_MINOR}.${KLEE_VERSION_PATCH}.${KLEE_VERSION_TWEAK}")
message(STATUS "KLEE version ${KLEE_VERSION}")
set(PACKAGE_STRING "\"KLEE ${KLEE_VERSION}\"")
set(PACKAGE_URL "\"https://klee.github.io\"")

################################################################################
# Set various useful variables depending on CMake version
################################################################################
if (("${CMAKE_VERSION}" VERSION_EQUAL "3.2") OR ("${CMAKE_VERSION}" VERSION_GREATER "3.2"))
  # In CMake >= 3.2 add_custom_command() supports a ``USES_TERMINAL`` argument
  set(ADD_CUSTOM_COMMAND_USES_TERMINAL_ARG "USES_TERMINAL")
else()
  set(ADD_CUSTOM_COMMAND_USES_TERMINAL_ARG "")
endif()

if (("${CMAKE_VERSION}" VERSION_EQUAL "3.4") OR ("${CMAKE_VERSION}" VERSION_GREATER "3.4"))
  # In CMake >= 3.4 ExternalProject_Add_Step() supports a `USES_TERMINAL` argument
  set(EXTERNAL_PROJECT_ADD_STEP_USES_TERMINAL_ARG "USES_TERMINAL" "1")
else()
  set(EXTERNAL_PROJECT_ADD_STEP_USES_TERMINAL_ARG "")
endif()

################################################################################
# Sanity check - Disallow building in source.
# Otherwise we would overwrite the Makefiles of the old build system.
################################################################################
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In source builds are not allowed. You should invoke "
          "CMake from a different directory.")
endif()

################################################################################
# Sanity Check: Check for in source build of the old build system.
# Some build files from the old build system could interfere with our build.
################################################################################
set(KLEE_OLD_BUILD_SYSTEM_FILES
  "include/klee/Config/config.h"
  "test/lit.site.cfg"
)
foreach (legacy_file ${KLEE_OLD_BUILD_SYSTEM_FILES})
  if (EXISTS "${CMAKE_SOURCE_DIR}/${legacy_file}")
    if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
      set(CLEAN_SRC_DIR_INSTRUCTIONS
        "The KLEE source tree apears to be a git repository so you can run"
        " \"git clean -dxn\" to see what files aren't part of the repo and then"
        " run \"git clean -fdx\" to remove them."
      )
    else()
      # This is the only reliable way to fix this.
      set(CLEAN_SRC_DIR_INSTRUCTIONS
        "The KLEE source tree doesn't appear to be a git repository so you will"
        " need to download a fresh copy of KLEE's source code."
      )
    endif()
    message(FATAL_ERROR "\"${CMAKE_SOURCE_DIR}/${legacy_file}\""
      " exists in KLEE's source tree. It is likely that the Autoconf/Makefile"
      " build system was configured to do an in-source build in KLEE's source"
      " tree. This could cause problems with the CMake build. "
      ${CLEAN_SRC_DIR_INSTRUCTIONS}
      " You can then run cmake again."
    )
  endif()
endforeach()

################################################################################
# Build type
################################################################################
message(STATUS "CMake generator: ${CMAKE_GENERATOR}")
if (DEFINED CMAKE_CONFIGURATION_TYPES)
  # Multi-configuration build (e.g. Xcode). Here
  # CMAKE_BUILD_TYPE doesn't matter
  message(STATUS "Available configurations: ${CMAKE_CONFIGURATION_TYPES}")
else()
  # Single configuration generator (e.g. Unix Makefiles, Ninja)
  set(available_build_types Debug Release RelWithDebInfo MinSizeRel)
  if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE is not set. Setting default")
    message(STATUS "The available build types are: ${available_build_types}")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE String
        "Options are ${available_build_types}"
        FORCE)
    # Provide drop down menu options in cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${available_build_types})
  endif()
  message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

  # Check the selected build type is valid
  list(FIND available_build_types "${CMAKE_BUILD_TYPE}" _build_type_index)
  if ("${_build_type_index}" EQUAL "-1")
    message(FATAL_ERROR "\"${CMAKE_BUILD_TYPE}\" is an invalid build type.\n"
      "Use one of the following build types ${available_build_types}")
  endif()
endif()


################################################################################
# Add our CMake module directory to the list of module search directories
################################################################################
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

################################################################################
# Assertions
################################################################################
option(ENABLE_KLEE_ASSERTS "Enable KLEE assertions" ON)
if (ENABLE_KLEE_ASSERTS)
  message(STATUS "KLEE assertions enabled")
  # Assume that -DNDEBUG isn't set.
else()
  message(STATUS "KLEE assertions disabled")
  list(APPEND KLEE_COMPONENT_CXX_DEFINES "NDEBUG")
endif()

################################################################################
# KLEE timestamps
################################################################################
option(KLEE_ENABLE_TIMESTAMP "Add timestamps to KLEE sources" OFF)

################################################################################
# Include useful CMake functions
################################################################################
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckPrototypeDefinition)
include("${CMAKE_SOURCE_DIR}/cmake/string_to_list.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/klee_component_add_cxx_flag.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/add_global_flag.cmake")

################################################################################
# Compiler flags for KLEE components
# Subsequent commands will append to these. These are used instead of
# directly modifying CMAKE_CXX_FLAGS so that other code can be easily built with
# different flags.
################################################################################
set(KLEE_COMPONENT_EXTRA_INCLUDE_DIRS "")
set(KLEE_COMPONENT_CXX_DEFINES "")
set(KLEE_COMPONENT_CXX_FLAGS "")
set(KLEE_SOLVER_LIBRARIES "")
set(KLEE_COMPONENT_EXTRA_LIBRARIES "")

################################################################################
# Find LLVM
################################################################################
include(${CMAKE_SOURCE_DIR}/cmake/find_llvm.cmake)
set(NEEDED_LLVM_VARS
  LLVM_PACKAGE_VERSION
  LLVM_VERSION_MAJOR
  LLVM_VERSION_MINOR
  LLVM_VERSION_PATCH
  LLVM_DEFINITIONS
  LLVM_ENABLE_ASSERTIONS
  LLVM_ENABLE_EH
  LLVM_ENABLE_RTTI
  LLVM_INCLUDE_DIRS
  LLVM_LIBRARY_DIRS
  LLVM_TOOLS_BINARY_DIR
  LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN
  TARGET_TRIPLE
)

foreach (vname ${NEEDED_LLVM_VARS})
  message(STATUS "${vname}: \"${${vname}}\"")
  if (NOT (DEFINED "${vname}"))
    message(FATAL_ERROR "${vname} was not defined")
  endif()
endforeach()

if (LLVM_ENABLE_ASSERTIONS)
  # Certain LLVM debugging macros only work when LLVM was built with asserts
  set(ENABLE_KLEE_DEBUG 1) # for config.h
else()
  unset(ENABLE_KLEE_DEBUG) # for config.h
endif()

if (LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN)
  list(APPEND KLEE_COMPONENT_CXX_FLAGS "-fvisibility-inlines-hidden")
endif()


list(APPEND KLEE_COMPONENT_CXX_DEFINES ${LLVM_DEFINITIONS})
list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})

# Find llvm-link
set(LLVM_LINK "${LLVM_TOOLS_BINARY_DIR}/llvm-link")
if (NOT EXISTS "${LLVM_LINK}")
  message(FATAL_ERROR "Failed to find llvm-link at \"${LLVM_LINK}\"")
endif()

# Find llvm-ar
set(LLVM_AR "${LLVM_TOOLS_BINARY_DIR}/llvm-ar")
if (NOT EXISTS "${LLVM_AR}")
  message(FATAL_ERROR "Failed to find llvm-ar at \"${LLVM_AR}\"")
endif()

# Find llvm-as
set(LLVM_AS "${LLVM_TOOLS_BINARY_DIR}/llvm-as")
if (NOT EXISTS "${LLVM_AS}")
  message(FATAL_ERROR "Failed to find llvm-as at \"${LLVM_AS}\"")
endif()

################################################################################
# Find bitcode compiler
################################################################################
include("${CMAKE_SOURCE_DIR}/cmake/find_bitcode_compiler.cmake")
message(STATUS "LLVMCC: ${LLVMCC}")
if (NOT EXISTS "${LLVMCC}")
  message(FATAL_ERROR "Cannot find C bitcode compiler \"${LLVMCC}\"")
endif()
message(STATUS "LLVMCXX: ${LLVMCXX}")
if (NOT EXISTS "${LLVMCXX}")
  message(FATAL_ERROR "Cannot find C++ bitcode compiler \"${LLVMCXX}\"")
endif()

################################################################################
# C++ version
################################################################################
option(USE_CXX11 "Use C++11" OFF)
if ("${LLVM_PACKAGE_VERSION}" VERSION_GREATER "3.5.0")
  message(STATUS "Using LLVM >= 3.5.0. Forcing using C++11")
  set(USE_CXX11 ON CACHE BOOL "Use C++11" FORCE)
endif()

if (USE_CXX11)
  message(STATUS "Enabling C++11")
  # FIXME: Use CMake's own mechanism for managing C++ version.
  # Set globally because it is unlikely we would want to compile
  # using mixed C++ versions.
  add_global_cxx_flag("-std=c++11" REQUIRED)
else()
  # This is needed because with GCC 6 the default changed from gnu++98 to
  # gnu++14.
  add_global_cxx_flag("-std=gnu++98" REQUIRED)
endif()

################################################################################
# Warnings
################################################################################
include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)

################################################################################
# Solvers
################################################################################

# STP: Use CMake facility to detect. The user can pass `-DSTP_DIR=` to force
# a particular directory.
option(ENABLE_SOLVER_STP "Enable STP solver support" OFF)
if (ENABLE_SOLVER_STP)
  message(STATUS "STP solver support enabled")
  # Although find_package() will set `STP_DIR` for us add it here so that
  # is displayed in `ccmake` and `cmake-gui`.
  set(STP_DIR "" CACHE PATH "Path to directory containing STPConfig.cmake")
  find_package(STP CONFIG)
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

# Z3
option(ENABLE_SOLVER_Z3 "Enable Z3 solver support" OFF)
if (ENABLE_SOLVER_Z3)
  message(STATUS "Z3 solver support enabled")
  find_package(Z3)
  if (Z3_FOUND)
    message(STATUS "Found Z3")
    set(ENABLE_Z3 1) # For config.h
    list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${Z3_INCLUDE_DIRS})
    list(APPEND KLEE_SOLVER_LIBRARIES ${Z3_LIBRARIES})

    # Check the signature of `Z3_get_error_msg()`
    set (_old_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} ${Z3_LIBRARIES})
    check_prototype_definition(Z3_get_error_msg
      "Z3_string Z3_get_error_msg(Z3_context c, Z3_error_code err)"
      "NULL" "${Z3_INCLUDE_DIRS}/z3.h" HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
    set(CMAKE_REQUIRED_LIBRARIES ${_old_CMAKE_REQUIRED_LIBRARIES})
    if (HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
      message(STATUS "Z3_get_error_msg requires context")
    else()
      message(STATUS "Z3_get_error_msg does not require context")
    endif()
  else()
    message(FATAL_ERROR "Z3 not found.")
  endif()
else()
  message(STATUS "Z3 solver support disabled")
  set(ENABLE_Z3 0) # For config.h
endif()

# metaSMT
option(ENABLE_SOLVER_METASMT "Enable metaSMT solver support" OFF)
if (ENABLE_SOLVER_METASMT)
  message(STATUS "metaSMT solver support enabled")
  set(ENABLE_METASMT 1)
  include(${CMAKE_SOURCE_DIR}/cmake/find_metasmt.cmake)
else()
  message(STATUS "metaSMT solver support disabled")
  set(ENABLE_METASMT 0) # For config.h
endif()

if ((NOT ${ENABLE_Z3}) AND (NOT ${ENABLE_STP}) AND (NOT ${ENABLE_METASMT}))
  message(FATAL_ERROR "No solver was specified. At least one solver is required."
    "You should enable a solver by passing one of more the following options"
    " to cmake:\n"
    "\"-DENABLE_SOLVER_STP=ON\"\n"
    "\"-DENABLE_SOLVER_Z3=ON\"\n"
    "\"-DENABLE_SOLVER_METASMT=ON\"")
endif()

###############################################################################
# Exception handling
###############################################################################
if (NOT LLVM_ENABLE_EH)
  if (ENABLE_SOLVER_METASMT)
    message(WARNING "Not disabling exceptions because metaSMT uses them")
  else()
    klee_component_add_cxx_flag("-fno-exceptions" REQUIRED)
  endif()
endif()

###############################################################################
# RTTI
###############################################################################
if (NOT LLVM_ENABLE_RTTI)
  if (ENABLE_SOLVER_METASMT AND metaSMT_REQUIRE_RTTI)
    message(FATAL_ERROR
      "RTTI cannot be disabled because metaSMT uses them."
      "This build configuration is not supported and will likely not work."
      "You should recompile LLVM with RTTI enabled.")
  else()
    klee_component_add_cxx_flag("-fno-rtti" REQUIRED)
  endif()
endif()

################################################################################
# Support for compressed logs
################################################################################
find_package(ZLIB)
if (${ZLIB_FOUND})
  set(HAVE_ZLIB_H 1) # For config.h
	set(TARGET_LIBS ${TARGET_LIBS} z)
  list(APPEND KLEE_COMPONENT_EXTRA_LIBRARIES ${ZLIB_LIBRARIES})
  list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS})
else()
  unset(HAVE_ZLIB_H) # For config.h
endif()

################################################################################
# TCMalloc support
################################################################################
OPTION(ENABLE_TCMALLOC "Enable TCMalloc support" OFF)
if (ENABLE_TCMALLOC)
  message(STATUS "TCMalloc support enabled")
  set(TCMALLOC_HEADER "gperftools/malloc_extension.h")
  check_include_file_cxx("${TCMALLOC_HEADER}" HAVE_GPERFTOOLS_MALLOC_EXTENSION_H)
  if (${HAVE_GPERFTOOLS_MALLOC_EXTENSION_H})
    find_library(TCMALLOC_LIBRARIES
      NAMES tcmalloc tcmalloc_minimal
      DOC "TCMalloc libraries"
    )
    if (NOT TCMALLOC_LIBRARIES)
      message(FATAL_ERROR
        "Found \"${TCMALLOC_HEADER}\" but could not find library")
    endif()
    list(APPEND KLEE_COMPONENT_EXTRA_LIBRARIES ${TCMALLOC_LIBRARIES})
    if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU"))
      # TCMalloc's documentation says its safest to pass these flags when
      # building with gcc because gcc can optimize assuming its using its own
      # malloc.
      klee_component_add_cxx_flag(-fno-builtin-malloc REQUIRED)
      klee_component_add_cxx_flag(-fno-builtin-calloc REQUIRED)
      klee_component_add_cxx_flag(-fno-builtin-realloc REQUIRED)
      klee_component_add_cxx_flag(-fno-builtin-free REQUIRED)
    endif()
  else()
    message(FATAL_ERROR "Can't find \"${TCMALLOC_HEADER}\"")
  endif()
else()
  unset(HAVE_GPERFTOOLS_MALLOC_EXTENSION_H)
  unset(HAVE_GPERFTOOLS_MALLOC_EXTENSION_H CACHE)
  message(STATUS "TCMalloc support disabled")
endif()

################################################################################
# Detect libcap
################################################################################
check_include_file("sys/capability.h" HAVE_SYS_CAPABILITY_H)
if (HAVE_SYS_CAPABILITY_H)
  find_library(LIBCAP_LIBRARIES
    NAMES cap
    DOC "libcap library"
  )
  if (NOT LIBCAP_LIBRARIES)
    message(FATAL_ERROR "Found \"sys/capability.h\" but could not find libcap")
  endif()
else()
  set(LIBCAP_LIBRARIES "")
endif()

################################################################################
# Detect libutil
################################################################################
check_include_file("pty.h" HAVE_PTY_H)
if (HAVE_PTY_H)
  find_library(LIBUTIL_LIBRARIES
    NAMES util
    DOC "libutil library. Typically part of glibc")
  if (NOT LIBUTIL_LIBRARIES)
    message(FATAL_ERROR "Found \"pty.h\" but could not find libutil")
  endif()
endif()

################################################################################
# Miscellaneous header file detection
################################################################################
check_function_exists(mallinfo HAVE_MALLINFO) # FIXME: should test CXX compiler not C
check_function_exists(__ctype_b_loc HAVE_CTYPE_EXTERNALS) # FIXME: should test CXX compiler not C

check_include_file_cxx(malloc/malloc.h HAVE_MALLOC_MALLOC_H)
check_function_exists(malloc_zone_statistics HAVE_MALLOC_ZONE_STATISTICS)

# FIXME: This is needed by the runtime not KLEE itself so we are testing the wrong
# compiler.
check_include_file("selinux/selinux.h" HAVE_SELINUX_SELINUX_H)
check_include_file("sys/acl.h" HAVE_SYS_ACL_H)
if (HAVE_SELINUX_SELINUX_H)
  message(STATUS "SELinux support enabled")
  set(HAVE_SELINUX 1)
  # Test what function signature we need to use for SELinux. The signatures
  # have changed between 2.2 and 2.3. In particular, the type of the "security
  # context" parameter was changed from char * to const char *, with this
  # patch: [PATCH] Get rid of security_context_t and fix const declarations.
  # [http://www.spinics.net/lists/selinux/msg14827.html]
  check_prototype_definition(setcon
    "int setcon(char* context)"
    "0"
    "selinux/selinux.h"
    SELINUX_SECURITY_CTX_NO_CONST
  )
  if (SELINUX_SECURITY_CTX_NO_CONST)
    message(STATUS "security_context_t is char*")
    set(KLEE_SELINUX_CTX_CONST " ")
  else()
    check_prototype_definition(setcon
      "int setcon(const char* context)"
      "0"
      "selinux/selinux.h"
      SELINUX_SECURITY_CTX_WITH_CONST
    )
    if (SELINUX_SECURITY_CTX_WITH_CONST)
      message(STATUS "security_context_t is const char*")
      set(KLEE_SELINUX_CTX_CONST "const")
    else()
      message(FATAL_ERROR "Failed to determine function signature for \"setcon\"")
    endif()
  endif()
else()
  message(STATUS "SELinux support disabled")
  set(HAVE_SELINUX 0)
endif()

################################################################################
# KLEE runtime support
################################################################################
# This is set here and not in `runtime` because `config.h` needs to be generated.


set(available_klee_runtime_build_types
  "Release"
  "Release+Debug"
  "Release+Asserts"
  "Release+Debug+Asserts"
  "Debug"
  "Debug+Asserts"
)
if (NOT KLEE_RUNTIME_BUILD_TYPE)
  message(STATUS "KLEE_RUNTIME_BUILD_TYPE is not set. Setting default")
  message(STATUS "The available runtime build types are: ${available_klee_runtime_build_types}")
  set(KLEE_RUNTIME_BUILD_TYPE "Release+Debug+Asserts" CACHE String
    "Options are ${available_klee_runtime_build_types}"
    FORCE)
endif()
# Provide drop down menu options in cmake-gui
set_property(CACHE
  KLEE_RUNTIME_BUILD_TYPE
  PROPERTY STRINGS ${available_klee_runtime_build_types})
message(STATUS "KLEE_RUNTIME_BUILD_TYPE: ${KLEE_RUNTIME_BUILD_TYPE}")

set(KLEE_INSTALL_RUNTIME_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/klee/runtime")

# Location where KLEE will look for the built runtimes by default.
set(KLEE_RUNTIME_DIRECTORY "${CMAKE_BINARY_DIR}/${KLEE_RUNTIME_BUILD_TYPE}/lib")

################################################################################
# KLEE POSIX Runtime Support
################################################################################
option(ENABLE_POSIX_RUNTIME "Enable KLEE's POSIX runtime" OFF)
if (ENABLE_POSIX_RUNTIME)
  message(STATUS "POSIX runtime enabled")
  if (NOT ENABLE_KLEE_UCLIBC)
    message(WARNING "Enabling POSIX runtime without klee-uclibc support."
      "The runtime might not work without it. Pass `-DENABLE_KLEE_UCLIBC=ON`"
      " to enable klee-uclibc support.")
  endif()
else()
  message(STATUS "POSIX runtime disabled")
endif()

################################################################################
# KLEE uclibc support
################################################################################
option(ENABLE_KLEE_UCLIBC "Enable support for klee-uclibc" OFF)
if (ENABLE_KLEE_UCLIBC)
  message(STATUS "klee-uclibc support enabled")
  set(SUPPORT_KLEE_UCLIBC 1) # For config.h
  set(KLEE_UCLIBC_PATH "" CACHE PATH "Path to klee-uclibc root directory")
  if (NOT IS_DIRECTORY "${KLEE_UCLIBC_PATH}")
    message(FATAL_ERROR
      "KLEE_UCLIBC_PATH (\"${KLEE_UCLIBC_PATH}\") is not a valid directory.\n"
      "Try passing -DKLEE_UCLIBC_PATH=<path> to cmake where <path> is the path"
      " to the root of the klee-uclibc directory.")
  endif()

  # Find the C library bitcode archive
  set(KLEE_UCLIBC_BCA_NAME "klee-uclibc.bca")
  set(KLEE_UCLIBC_C_BCA "${KLEE_UCLIBC_PATH}/lib/libc.a")
  if (NOT EXISTS "${KLEE_UCLIBC_C_BCA}")
    message(FATAL_ERROR
      "klee-uclibc library not found at \"${KLEE_UCLIBC_C_BCA}\"")
  endif()
  message(STATUS "Found klee-uclibc library: \"${KLEE_UCLIBC_C_BCA}\"")

  # Make a symlink to KLEE_UCLIBC_C_BCA so KLEE can find it where it
  # is expected.
  file(MAKE_DIRECTORY "${KLEE_RUNTIME_DIRECTORY}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
    "${KLEE_UCLIBC_C_BCA}"
    "${KLEE_RUNTIME_DIRECTORY}/${KLEE_UCLIBC_BCA_NAME}"
  )
  list(APPEND KLEE_COMPONENT_CXX_DEFINES
    -DKLEE_UCLIBC_BCA_NAME=\"${KLEE_UCLIBC_BCA_NAME}\")

  # Add klee-uclibc to the install target. We install the original
  # file rather than the symlink because CMake would just copy the symlink
  # rather than the file.
  install(FILES "${KLEE_UCLIBC_C_BCA}"
    DESTINATION "${KLEE_INSTALL_RUNTIME_DIR}"
    RENAME "${KLEE_UCLIBC_BCA_NAME}"
    )

else()
  message(STATUS "klee-uclibc support disabled")
  set(SUPPORT_KLEE_UCLIBC 0) # For config.h
endif()

################################################################################
# Generate `config.h`
################################################################################
configure_file(${CMAKE_SOURCE_DIR}/include/klee/Config/config.h.cmin
  ${CMAKE_BINARY_DIR}/include/klee/Config/config.h)

################################################################################
# Generate `CompileTimeInfo.h`
################################################################################
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
  # Get information from git. We use third-party code to do this. The nice
  # thing about this code is it will trigger a re-configure if the HEAD changes
  # which means when we build KLEE, it should always have the correct git
  # information.
  include(${CMAKE_SOURCE_DIR}/cmake/GetGitRevisionDescription.cmake)
  get_git_head_revision(_NOT_USED_KLEE_GIT_REFSPEC KLEE_GIT_SHA1HASH)
  message(STATUS "KLEE_GIT_SHA1HASH: ${KLEE_GIT_SHA1HASH}")
else()
  set(KLEE_GIT_SHA1HASH "unknown")
endif()
set(AUTO_GEN_MSG "AUTOMATICALLY GENERATED. DO NOT EDIT!")
configure_file(${CMAKE_SOURCE_DIR}/include/klee/Config/CompileTimeInfo.h.cmin
  ${CMAKE_BINARY_DIR}/include/klee/Config/CompileTimeInfo.h
)

################################################################################
# Global include directories
################################################################################
include_directories("${CMAKE_BINARY_DIR}/include")
include_directories("${CMAKE_SOURCE_DIR}/include")

################################################################################
# Set default location for targets in the build directory
################################################################################
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################################################################################
# KLEE components
################################################################################
include("${CMAKE_SOURCE_DIR}/cmake/klee_add_component.cmake")
add_subdirectory(lib)
add_subdirectory(runtime)

################################################################################
# KLEE tools
################################################################################
add_subdirectory(tools)

################################################################################
# Testing
################################################################################
option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
option(ENABLE_SYSTEM_TESTS "Enable system tests" ON)

# This provides a migration path for older build directories that have this
# variable set in their cache. Leaving it behind could lead to confusion so
# removing it is probably a good idea.
# TODO: Remove this eventually (probably next release or something).
if (DEFINED ENABLE_TESTS)
  message(WARNING "You have the \"ENABLE_TESTS\" variable is your CMake cache."
    "This variable no longer has any meaning so removing it from your cache")
  unset(ENABLE_TESTS CACHE)
endif()

if (ENABLE_UNIT_TESTS OR ENABLE_SYSTEM_TESTS)
  message(STATUS "Testing is enabled")

  # Find lit
  set(LIT_TOOL_NAMES "llvm-lit" "lit")
  find_program(
    LIT_TOOL
    NAMES ${LIT_TOOL_NAMES}
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    DOC "Path to lit tool"
  )

  set(LIT_ARGS
    "-v;-s"
    CACHE
    STRING
    "Lit arguments"
  )

  if ((NOT LIT_TOOL) OR (NOT EXISTS "${LIT_TOOL}"))
    message(FATAL_ERROR "The lit tool is required for testing."
      " CMake tried to find lit with the following names \"${LIT_TOOL_NAMES}\""
      " but it could not be found.\n"
      "You should either disable testing by passing "
      "\"-DENABLE_UNIT_TESTS=OFF -DENABLE_SYSTEM_TESTS=OFF\" to cmake"
      " or you should install the lit tool from the Python Package Index by running"
      " \"pip install lit\". Note \"pip\" requires root privileges to run. If you"
      " don't have root privileges you can create a virtual python environment using"
      " the \"virtualenv\" tool and run \"pip\" from there.")
  else()
    message(STATUS "Using lit: ${LIT_TOOL}")
  endif()

  # Add global test target
  add_custom_target(check
    COMMENT "Running tests"
  )
else()
  message(STATUS "Testing is disabled")
endif()


if (ENABLE_UNIT_TESTS)
  message(STATUS "Unit tests enabled")
  add_subdirectory(unittests)
  add_dependencies(check unittests)
else()
  message(STATUS "Unit tests disabled")
endif()
if (ENABLE_SYSTEM_TESTS)
  message(STATUS "System tests enabled")
  add_subdirectory(test)
  add_dependencies(check systemtests)
else()
  message(STATUS "System tests disabled")
endif()

################################################################################
# Documentation
################################################################################
option(ENABLE_DOCS "Enable building documentation" ON)
if (ENABLE_DOCS)
  add_subdirectory(docs)
endif()

################################################################################
# Miscellaneous install
################################################################################
install(FILES include/klee/klee.h DESTINATION include/klee)
