#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

function(add_vanilla_googletest_subdirectory directory)
  # Prevent Google Test from adding to our install target (Google Test 1.8.0+)
  # However, this can only be disabled starting with 1.8.1 (a.k.a. 1.9.0)
  set(INSTALL_GTEST OFF)

  # Google Mock is currently not used by our tests
  set(BUILD_GMOCK OFF)

  # (only) Google Test 1.8.0 with BUILD_GMOCK=OFF needs BUILD_GTEST=ON
  set(BUILD_GTEST ON)

  # Make option() in subdirectory respect normal variables (as set above).
  # NOTE: The enclosing function limits the scope of this policy setting.
  # FIXME: Remove once all supported Google Test versions require CMake 3.13+
  #        (or set policy CMP0077 to NEW by themselves)
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

  # include Google Test's CMakeLists.txt
  add_subdirectory(${directory} "${CMAKE_CURRENT_BINARY_DIR}/gtest_build")
endfunction()

function(ensure_valid_llvm_gtest_target)
  block(SCOPE_FOR VARIABLES)
  if ("${LLVM_VERSION_MAJOR}" LESS 13)
    list(FIND LLVM_EXPORTED_TARGETS "gtest" _GTEST_INDEX)
    if (${_GTEST_INDEX} GREATER -1)
      return()
    endif()
  else()
    # use that LLVM's Google Test always depends on LLVM's own support library

    # if LLVM was built using "BUILD_SHARED_LIBS=ON", we need to collect
    # IMPORTED_LINK_DEPENDENT_LIBRARIES[_<CONFIGURATION>] for the target
    get_target_property(_GTEST_DEPENDENCIES gtest
      IMPORTED_LINK_DEPENDENT_LIBRARIES)
    if (NOT _GTEST_DEPENDENCIES)
      set(_GTEST_DEPENDENCIES "")
    endif()
    get_target_property(_GTEST_CONFIGS gtest IMPORTED_CONFIGURATIONS)
    foreach(_GTEST_CONFIG "${_GTEST_CONFIGS}")
      get_target_property(_GTEST_DEP_CONFIG gtest
        IMPORTED_LINK_DEPENDENT_LIBRARIES_${_GTEST_CONFIG})
      if (_GTEST_DEP_CONFIG)
        list(APPEND _GTEST_DEPENDENCIES "${_GTEST_DEP_CONFIG}")
      endif()
    endforeach()

    # PUBLIC and INTERFACE link dependencies (when LLVM uses static linking)
    get_target_property(_GTEST_LINK_LIBS gtest INTERFACE_LINK_LIBRARIES)
    if (_GTEST_LINK_LIBS)
        list(APPEND _GTEST_DEPENDENCIES "${_GTEST_LINK_LIBS}")
    endif()

    # determine the name of the library offering LLVM's support
    if ("${LLVM_LINK_LLVM_DYLIB}")
      set(_SUPPORT_DEPENDENCY "LLVM")
    else()
      llvm_map_components_to_libnames(_SUPPORT_DEPENDENCY support)
    endif()

    # check if this support library is among the dependencies of gtest
    list(FIND _GTEST_DEPENDENCIES "${_SUPPORT_DEPENDENCY}" _SUPPORT_INDEX)
    if (${_SUPPORT_INDEX} GREATER -1)
      return()
    endif()
  endif()

  message(FATAL_ERROR "An existing 'gtest' CMake target was imported. This "
    "prevents KLEE from defining its own 'gtest' target (for Google Test).\n"
    "KLEE has support for reusing the 'gtest' target exported by LLVM, but "
    "the 'gtest' target imported does not appear to originate from LLVM.\n"
    "Please make sure that no KLEE dependency (included with `find_package`) "
    "exports a 'gtest' target in your configuration.")
  endblock()
endfunction()

if (TARGET gtest)
  # Google Test target is already defined, we cannot include Google Test twice.
  # Thus, we try to reuse the 'gtest' target if it originates from LLVM.
  # Otherwise, we fail with an error (and ask user to fix their configuration).

  message(STATUS "Google Test: Reusing 'gtest' target.")
  ensure_valid_llvm_gtest_target()

  message(WARNING "LLVM exports its 'gtest' CMake target (for Google Test), "
    "which is imported by KLEE (via `find_package`). This prevents KLEE from "
    "defining its own 'gtest' target (for Google Test).\n"
    "Thus, KLEE will reuse the imported 'gtest' target from LLVM. This is, "
    "however, only recommended if LLVM and KLEE are built using the same "
    "compiler and linker flags (to prevent compatibility issues).\n"
    "To prevent CMake from reusing the target or to use a different version "
    "of Google Test, try either of the following:\n"
    "- Point LLVM_DIR to the directory containing the `LLVMConfig.cmake` file "
    "of an installed copy of LLVM instead of a build tree.\n"
    "- Pass -DLLVM_INCLUDE_TESTS=OFF to the CMake invocation used for building "
    "LLVM. This prevents building unit tests in LLVM (but not in KLEE) and "
    "exporting the target from LLVM's build tree.")

  if (GTEST_SRC_DIR)
    message(FATAL_ERROR "Cannot use GTEST_SRC_DIR when target 'gtest' is"
      "already defined.\n"
      "Either let KLEE reuse LLVM's Google Test setup by not setting "
      "GTEST_SRC_DIR explicitly or choose one of the options above to prevent"
      "LLVM from exporting this target.")
  endif()

  # older LLVM versions do not set INTERFACE_INCLUDE_DIRECTORIES for gtest
  if ("${LLVM_VERSION_MAJOR}" LESS 12)
    if (IS_DIRECTORY "${LLVM_BUILD_MAIN_SRC_DIR}")
      target_include_directories(gtest
        INTERFACE
        "${LLVM_BUILD_MAIN_SRC_DIR}/utils/unittest/googletest/include"
      )
    endif()
  endif()
else()
  # LLVM's 'gtest' target is not reused

  if (NOT GTEST_SRC_DIR)
    if (IS_DIRECTORY "${LLVM_BUILD_MAIN_SRC_DIR}")
      # build from LLVM's utils directory
      message(STATUS "Google Test: Building from LLVM's source tree.")


      # LLVM replaced Google Test's CMakeLists.txt with its own, which requires
      # add_llvm_library() from AddLLVM.cmake.
      list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
      include(AddLLVM)

      add_subdirectory("${LLVM_BUILD_MAIN_SRC_DIR}/utils/unittest/"
        "${CMAKE_CURRENT_BINARY_DIR}/gtest_build")

      # older LLVM versions do not set INTERFACE_INCLUDE_DIRECTORIES for gtest
      if ("${LLVM_VERSION_MAJOR}" LESS 12)
        target_include_directories(gtest
          INTERFACE
          "${LLVM_BUILD_MAIN_SRC_DIR}/utils/unittest/googletest/include"
        )
      endif()

      # add includes for LLVM's modifications
      target_include_directories(gtest BEFORE PRIVATE ${LLVM_INCLUDE_DIRS})
      # we cannot disable gtest_main, but will not use it later
      target_include_directories(gtest_main BEFORE PRIVATE ${LLVM_INCLUDE_DIRS})
    endif()

    # try using system installation of GTest instead
    find_package(GTest QUIET)
    if (GTest_FOUND)
      message(STATUS "Found GTest ${GTest_VERSION}")
    else()
      # try to find Google Test, as GTEST_SRC_DIR is not manually specified
      find_path(GTEST_SRC_DIR
        "src/gtest.cc"

        HINTS
        "/usr/src/gtest"

        # prevent CMake from finding gtest.cc in LLVM's utils directory
        NO_DEFAULT_PATH

        DOC
        "Path to Google Test source directory"
      )
    endif()
  endif()

  if (NOT TARGET gtest AND NOT GTest_FOUND)
    # building from GTEST_SRC_DIR, not from LLVM's utils directory
    find_path(GTEST_INCLUDE_DIR
      "gtest/gtest.h"

      HINTS
      "${GTEST_SRC_DIR}/include"
      "${GTEST_SRC_DIR}/googletest/include"

      NO_DEFAULT_PATH

      DOC
      "Path to Google Test include directory"
    )

    if (NOT EXISTS "${GTEST_SRC_DIR}")
      message(FATAL_ERROR "Google Test source directory cannot be found.\n"
      "Try passing -DGTEST_SRC_DIR=<path_to_gtest_source> to CMake where "
      "<path_to_gtest_source> is the path to the Google Test source tree.\n"
      "Alternatively, you can disable unit tests by passing "
      "-DENABLE_UNIT_TESTS=OFF to CMake.")
    endif()
    message(STATUS "Google Test: Building from source.")
    message(STATUS "GTEST_SRC_DIR: ${GTEST_SRC_DIR}")

    add_vanilla_googletest_subdirectory(${GTEST_SRC_DIR})

    # Google Test versions < 1.8.0 do not set INTERFACE_INCLUDE_DIRECTORIES
    target_include_directories(gtest
      INTERFACE
      ${GTEST_INCLUDE_DIR}
    )
  endif()

  if (NOT GTest_FOUND)
    # build Google Test with KLEE's defines and compile flags
    target_compile_definitions(gtest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
    target_compile_options(gtest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
  endif()
endif()

enable_testing()


# This keeps track of all the unit test
# targets so we can ensure they are built
# before trying to run them.
define_property(GLOBAL
  PROPERTY KLEE_UNIT_TEST_TARGETS
  BRIEF_DOCS "KLEE unit tests"
  FULL_DOCS "KLEE unit tests"
)

if (TARGET gtest)
  set(GTEST_TARGET_NAME gtest)
elseif (TARGET GTest::gtest)
  set(GTEST_TARGET_NAME GTest::gtest)
elseif (TARGET GTest::GTest)
  set(GTEST_TARGET_NAME GTest::GTest)
else()
  message(FATAL_ERROR
    "Cannot determine name of the Google Test CMake target (tried 'gtest', \
    'GTest::gtest' and 'GTest::GTest')")
endif()

add_library(unittest_main)
target_sources(unittest_main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/TestMain.cpp")
llvm_config(unittest_main "${USE_LLVM_SHARED}" support)

target_link_libraries(unittest_main
  PUBLIC
  ${GTEST_TARGET_NAME}
)
target_include_directories(unittest_main
  INTERFACE
  ${KLEE_COMPONENT_EXTRA_INCLUDE_DIRS}

  PRIVATE
  ${LLVM_INCLUDE_DIRS}
)
target_compile_definitions(unittest_main PUBLIC ${KLEE_COMPONENT_CXX_DEFINES})
target_compile_options(unittest_main PUBLIC ${KLEE_COMPONENT_CXX_FLAGS})

function(add_klee_unit_test target_name)
  add_executable(${target_name} ${ARGN})
  target_link_libraries(${target_name} PRIVATE unittest_main)
  set_target_properties(${target_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/unittests/"
  )
  set_property(GLOBAL
    APPEND
    PROPERTY KLEE_UNIT_TEST_TARGETS
    ${target_name}
  )
endfunction()

# Unit Tests
add_subdirectory(Annotations)
add_subdirectory(Assignment)
add_subdirectory(Expr)
add_subdirectory(Ref)
add_subdirectory(Solver)
add_subdirectory(Searcher)
add_subdirectory(TreeStream)
add_subdirectory(DiscretePDF)
add_subdirectory(Time)
add_subdirectory(RNG)

# Set up lit configuration
set (UNIT_TEST_EXE_SUFFIX "Test")
configure_file(lit-unit-tests-common.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  @ONLY)

# Add a target to run all the unit tests using lit
get_property(UNIT_TEST_DEPENDS
  GLOBAL
  PROPERTY KLEE_UNIT_TEST_TARGETS
)
add_custom_target(unittests
  COMMAND
    "${LIT_TOOL}" ${LIT_ARGS} "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${UNIT_TEST_DEPENDS}
    COMMENT "Running unittests"
    USES_TERMINAL
)
