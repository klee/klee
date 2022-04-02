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
cmake_minimum_required(VERSION 3.9.0)
if (POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
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

include(CheckFunctionExists)
include(CheckLibraryExists)

###############################################################################
# Project version
###############################################################################
set(KLEE_VERSION_MAJOR 2)
set(KLEE_VERSION_MINOR 3)
set(KLEE_VERSION "${KLEE_VERSION_MAJOR}.${KLEE_VERSION_MINOR}")

# If a patch is needed, we can add KLEE_VERSION_PATCH
# set(KLEE_VERSION_PATCH 0)
# set(KLEE_VERSION "${KLEE_VERSION_MAJOR}.${KLEE_VERSION_MINOR}.${KLEE_VERSION_PATCH}")

message(STATUS "KLEE version ${KLEE_VERSION}")
set(PACKAGE_STRING "\"KLEE ${KLEE_VERSION}\"")
set(PACKAGE_URL "\"https://klee.github.io\"")

################################################################################
# Sanity check - Disallow building in source.
################################################################################
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In source builds are not allowed. You should invoke "
          "CMake from a different directory.")
endif()

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
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
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


# Reference specific library paths used during linking for install
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

################################################################################
# Add our CMake module directory to the list of module search directories
################################################################################
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

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
# Assertions
################################################################################
option(ENABLE_KLEE_ASSERTS "Enable KLEE assertions" ON)
if (ENABLE_KLEE_ASSERTS)
  message(STATUS "KLEE assertions enabled")
  # Assume that -DNDEBUG isn't set.
else()
  message(STATUS "KLEE assertions disabled")
  list(APPEND KLEE_COMPONENT_CXX_DEFINES "-DNDEBUG")
endif()

################################################################################
# KLEE timestamps
################################################################################
option(KLEE_ENABLE_TIMESTAMP "Add timestamps to KLEE sources" OFF)

################################################################################
# Include useful CMake functions
################################################################################
include(GNUInstallDirs)
include(CheckCXXSymbolExists)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckPrototypeDefinition)
include(CMakePushCheckState)
include("${CMAKE_SOURCE_DIR}/cmake/string_to_list.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/klee_component_add_cxx_flag.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/add_global_flag.cmake")


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

set(OPTIONAL_LLVM_VARS
  LLVM_BUILD_MAIN_SRC_DIR
)
foreach (vname ${OPTIONAL_LLVM_VARS})
  if (${vname})
    message(STATUS "${vname}: \"${${vname}}\"")
  endif()
endforeach()

if (LLVM_ENABLE_ASSERTIONS)
  # Certain LLVM debugging macros only work when LLVM was built with asserts
  set(ENABLE_KLEE_DEBUG 1) # for config.h
else()
  unset(ENABLE_KLEE_DEBUG) # for config.h
endif()

# Warn about mixing build types.
# This is likely a bad idea because some of LLVM's header files use the NDEBUG
# macro which can change things like data layout.
if (LLVM_ENABLE_ASSERTIONS AND (NOT ENABLE_KLEE_ASSERTS))
  message(WARNING
    "LLVM was built with assertions but KLEE will be built without them.\n"
    "This might lead to unexpected behaviour."
  )
elseif ((NOT LLVM_ENABLE_ASSERTIONS) AND ENABLE_KLEE_ASSERTS)
  message(WARNING
    "LLVM was built without assertions but KLEE will be built with them.\n"
    "This might lead to unexpected behaviour."
  )
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
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

################################################################################
# Warnings
################################################################################
include(${CMAKE_SOURCE_DIR}/cmake/compiler_warnings.cmake)

################################################################################
# Solvers
################################################################################
# STP
include(${CMAKE_SOURCE_DIR}/cmake/find_stp.cmake)
# Z3
include(${CMAKE_SOURCE_DIR}/cmake/find_z3.cmake)
# metaSMT
include(${CMAKE_SOURCE_DIR}/cmake/find_metasmt.cmake)

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
if (ZLIB_FOUND)
  set(ENABLE_ZLIB_DEFAULT ON)
else()
  set(ENABLE_ZLIB_DEFAULT OFF)
endif()
option(ENABLE_ZLIB "Enable use of zlib" ${ENABLE_ZLIB_DEFAULT})
if (ENABLE_ZLIB)
  message(STATUS "Zlib support enabled")
  if (ZLIB_FOUND)
    set(HAVE_ZLIB_H 1) # For config.h
    set(TARGET_LIBS ${TARGET_LIBS} z)
    list(APPEND KLEE_COMPONENT_EXTRA_LIBRARIES ${ZLIB_LIBRARIES})
    list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS})
  else()
    message(FATAL_ERROR "ENABLE_ZLIB is true but zlib could not be found")
  endif()
else()
  message(STATUS "Zlib support disabled")
  unset(HAVE_ZLIB_H) # For config.h
endif()

################################################################################
# TCMalloc support
################################################################################
OPTION(ENABLE_TCMALLOC "Enable TCMalloc support" ON)
if (ENABLE_TCMALLOC)
  message(STATUS "TCMalloc support enabled")
  set(TCMALLOC_HEADER "gperftools/malloc_extension.h")
  find_path(TCMALLOC_INCLUDE_DIR "${TCMALLOC_HEADER}")
  cmake_push_check_state()
  set(CMAKE_REQUIRED_INCLUDES "${TCMALLOC_INCLUDE_DIR}")
  check_include_file_cxx("${TCMALLOC_HEADER}" HAVE_GPERFTOOLS_MALLOC_EXTENSION_H)
  cmake_pop_check_state()
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
    list(APPEND KLEE_COMPONENT_EXTRA_INCLUDE_DIRS ${TCMALLOC_INCLUDE_DIR})
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
# Detect SQLite3
################################################################################
find_package(SQLite3)
if (SQLITE3_FOUND)
  include_directories(${SQLITE3_INCLUDE_DIRS})
else()
    message( FATAL_ERROR "SQLite3 not found, please install" )
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
# On FreeBSD <sys/capability.h> is a different thing
  if (NOT LIBCAP_LIBRARIES OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(HAVE_SYS_CAPABILITY_H 0)
  endif()
else()
  set(LIBCAP_LIBRARIES "")
endif()

################################################################################
# Detect libutil
################################################################################
check_include_file(pty.h HAVE_PTY_H)
check_include_file(util.h HAVE_UTIL_H)
check_include_file(libutil.h HAVE_LIBUTIL_H)
if (HAVE_PTY_H OR HAVE_UTIL_H OR HAVE_LIBUTIL_H)
  check_function_exists(openpty openpty_in_libc)
  if (NOT openpty_in_libc)
    check_library_exists(util openpty "" openpty_in_libutil)
    if (openpty_in_libutil)
      set(LIBUTIL_LIBRARIES util)
    else ()
      message(FATAL_ERROR "Could not find libutil")
    endif (openpty_in_libutil)
  endif (NOT openpty_in_libc)
endif (HAVE_PTY_H OR HAVE_UTIL_H OR HAVE_LIBUTIL_H)

################################################################################
# Miscellaneous header file detection
################################################################################
check_cxx_symbol_exists(__ctype_b_loc ctype.h HAVE_CTYPE_EXTERNALS)
check_cxx_symbol_exists(mallinfo malloc.h HAVE_MALLINFO)
check_cxx_symbol_exists(malloc_zone_statistics malloc/malloc.h HAVE_MALLOC_ZONE_STATISTICS)

check_include_file(sys/statfs.h HAVE_SYSSTATFS_H)

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
# Workarounds
################################################################################
include(${CMAKE_SOURCE_DIR}/cmake/workaround_llvm_pr39177.cmake)

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
  set(KLEE_RUNTIME_BUILD_TYPE "Debug+Asserts" CACHE STRING
    "Options are ${available_klee_runtime_build_types}"
    FORCE)
endif()
# Provide drop down menu options in cmake-gui
set_property(CACHE
  KLEE_RUNTIME_BUILD_TYPE
  PROPERTY STRINGS ${available_klee_runtime_build_types})
message(STATUS "KLEE_RUNTIME_BUILD_TYPE: ${KLEE_RUNTIME_BUILD_TYPE}")

# Check availability of 32bit support for platform
include(CheckCSourceRuns)
cmake_push_check_state()
set(CMAKE_REQUIRED_FLAGS "-m32")
check_c_source_runs("int main(int argc, char** argv){return 0;}" M32_SUPPORTED)
cmake_pop_check_state()
message(STATUS "32bit platform supported: ${M32_SUPPORTED}")

set(KLEE_INSTALL_RUNTIME_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/klee/runtime")

# Location where KLEE will look for the built runtimes by default.
set(KLEE_RUNTIME_DIRECTORY "${CMAKE_BINARY_DIR}/runtime/lib")

file(MAKE_DIRECTORY ${KLEE_RUNTIME_DIRECTORY})

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

  # Copy KLEE_UCLIBC_C_BCA so KLEE can find it where it is expected.
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy
    "${KLEE_UCLIBC_C_BCA}"
    "${KLEE_RUNTIME_DIRECTORY}/${KLEE_UCLIBC_BCA_NAME}"
  )
  list(APPEND KLEE_COMPONENT_CXX_DEFINES
    -DKLEE_UCLIBC_BCA_NAME=\"${KLEE_UCLIBC_BCA_NAME}\")

else()
  message(STATUS "klee-uclibc support disabled")
  set(SUPPORT_KLEE_UCLIBC 0) # For config.h
endif()

################################################################################
# KLEE libc++ support
################################################################################
option(ENABLE_KLEE_LIBCXX "Enable libc++ for klee" OFF)
if (ENABLE_KLEE_LIBCXX)
  message(STATUS "klee-libc++ support enabled")
  set(SUPPORT_KLEE_LIBCXX 1) # For config.h
  set(KLEE_LIBCXX_DIR "" CACHE PATH "Path to directory containing libc++ shared object (bitcode)")
  if (NOT EXISTS "${KLEE_LIBCXX_DIR}")
    message(FATAL_ERROR
      "KLEE_LIBCXX_DIR (\"${KLEE_LIBCXX_DIR}\") does not exist.\n"
      "Try passing -DKLEE_LIBCXX_DIR=<path> to CMake where <path> is the path"
      "to the directory containing the libc++ shared object file (as bitcode).")
  endif()

  set(KLEE_LIBCXX_INCLUDE_DIR "" CACHE PATH "Path to libc++ include directory")
  if (NOT EXISTS "${KLEE_LIBCXX_INCLUDE_DIR}")
    message(FATAL_ERROR
      "KLEE_LIBCXX_INCLUDE_DIR (\"${KLEE_LIBCXX_INCLUDE_DIR}\") does not exist.\n"
      "Try passing -DKLEE_LIBCXX_INCLUDE_DIR=<path> to CMake where <path> is the"
      "libc++ include directory.")
  endif()
  message(STATUS "Use libc++ include path: \"${KLEE_LIBCXX_INCLUDE_DIR}\"")

  # Find the library bitcode archive

  # Check for static first
  set(KLEE_LIBCXX_BC_NAME "libc++.bca")
  set(KLEE_LIBCXX_BC_PATH "${KLEE_LIBCXX_DIR}/lib/${KLEE_LIBCXX_BC_NAME}")
  if (NOT EXISTS "${KLEE_LIBCXX_BC_PATH}")
    # Check for dynamic so lib
    set(KLEE_LIBCXX_BC_NAME "libc++.so.bc")
    set(KLEE_LIBCXX_BC_PATH "${KLEE_LIBCXX_DIR}/lib/${KLEE_LIBCXX_BC_NAME}")
    if (NOT EXISTS "${KLEE_LIBCXX_BC_PATH}")
      set(KLEE_LIBCXX_BC_NAME "libc++.dylib.bc")
      set(KLEE_LIBCXX_BC_PATH "${KLEE_LIBCXX_DIR}/lib/${KLEE_LIBCXX_BC_NAME}")
      if (NOT EXISTS "${KLEE_LIBCXX_BC_PATH}")
        message(FATAL_ERROR
          "libc++ library not found at \"${KLEE_LIBCXX_DIR}\"")
      endif()
    endif()
  endif()
  message(STATUS "Found libc++ library: \"${KLEE_LIBCXX_BC_PATH}\"")

  # Copy KLEE_LIBCXX_BC_PATH so KLEE can find it where it is expected.
  file(MAKE_DIRECTORY "${KLEE_RUNTIME_DIRECTORY}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy
    "${KLEE_LIBCXX_BC_PATH}"
    "${KLEE_RUNTIME_DIRECTORY}/${KLEE_LIBCXX_BC_NAME}"
  )
  list(APPEND KLEE_COMPONENT_CXX_DEFINES
    -DKLEE_LIBCXX_BC_NAME=\"${KLEE_LIBCXX_BC_NAME}\")

else()
  message(STATUS "libc++ support disabled")
  set(SUPPORT_KLEE_LIBCXX 0) # For config.h
endif()

################################################################################
# KLEE Exception Handling Support for C++
################################################################################
option(ENABLE_KLEE_EH_CXX "Enable support for C++ Exceptions" OFF)
if (ENABLE_KLEE_EH_CXX)
  if (NOT ENABLE_KLEE_LIBCXX)
    message(FATAL_ERROR "C++ Exception support requires klee-libc++."
      "Pass -DENABLE_KLEE_LIBCXX=ON to CMake to enable it.")
  endif()

  set(KLEE_LIBCXXABI_SRC_DIR "" CACHE PATH "Path to libc++abi source directory")
  if (NOT EXISTS "${KLEE_LIBCXXABI_SRC_DIR}")
    message(FATAL_ERROR
      "KLEE_LIBCXXABI_SRC_DIR (\"${KLEE_LIBCXXABI_SRC_DIR}\") does not exist.\n"
      "Try passing -DKLEE_LIBCXXABI_SRC_DIR=<path> to CMake where <path> is the"
      "libc++abi source directory.")
  endif()
  message(STATUS "Use libc++abi source path: \"${KLEE_LIBCXXABI_SRC_DIR}\"")

  if (${LLVM_VERSION_MAJOR} GREATER 8 OR ${LLVM_VERSION_MAJOR} EQUAL 8)
    set(SUPPORT_KLEE_EH_CXX 1) # For config.h
    message(STATUS "Support for C++ Exceptions enabled")
  else()
    set(SUPPORT_KLEE_EH_CXX 0) # For config.h
    message(STATUS "Support for C++ Exceptions (only available for LLVM >= 8.0.0) disabled")
  endif()

else()
  set(SUPPORT_KLEE_EH_CXX 0) # For config.h
endif()

################################################################################
# Sanitizer support
################################################################################
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
set(IS_ASAN_BUILD 0)
set(IS_UBSAN_BUILD 0)
set(IS_MSAN_BUILD 0)
string(REPLACE " " ";" _flags ${CMAKE_CXX_FLAGS})
foreach(arg IN ITEMS ${_flags})
  if (${arg} STREQUAL -fsanitize=address)
    set(IS_ASAN_BUILD 1)
  elseif (${arg} STREQUAL -fsanitize=undefined)
    set(IS_UBSAN_BUILD 1)
  elseif (${arg} STREQUAL -fsanitize=memory)
    set(IS_MSAN_BUILD 1)
  endif()
endforeach()
unset(_flags)

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
set(KLEE_UTILS_DIR ${CMAKE_SOURCE_DIR}/utils)


################################################################################
# Report the value of various variables to aid debugging
################################################################################
message(STATUS "KLEE_COMPONENT_EXTRA_INCLUDE_DIRS: '${KLEE_COMPONENT_EXTRA_INCLUDE_DIRS}'")
message(STATUS "KLEE_COMPONENT_CXX_DEFINES: '${KLEE_COMPONENT_CXX_DEFINES}'")
message(STATUS "KLEE_COMPONENT_CXX_FLAGS: '${KLEE_COMPONENT_CXX_FLAGS}'")
message(STATUS "KLEE_COMPONENT_EXTRA_LIBRARIES: '${KLEE_COMPONENT_EXTRA_LIBRARIES}'")
message(STATUS "KLEE_SOLVER_LIBRARIES: '${KLEE_SOLVER_LIBRARIES}'")

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
option(ENABLE_UNIT_TESTS "Enable unit tests" OFF)
option(ENABLE_SYSTEM_TESTS "Enable system tests" ON)

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

################################################################################
# Uninstall rule
################################################################################
configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
	@ONLY
)

add_custom_target(uninstall
  COMMAND
  "${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
  COMMENT "Uninstalling..."
  VERBATIM
)
