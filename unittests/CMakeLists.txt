#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

function(add_vanilla_googletest_subdirectory directory)
  if (POLICY CMP0077)
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
  else()
    # FIXME: Remove with CMake minimum version 3.13+
    # Use cache variable workaround as option() ignores normal variables

    # Prevent Google Test from adding to our install target (Google Test 1.8.0+)
    # However, this can only be disabled starting with 1.8.1 (a.k.a. 1.9.0)
    set(INSTALL_GTEST OFF CACHE BOOL "disable installing Google Test" FORCE)

    # Google Mock is currently not used by our tests
    set(BUILD_GMOCK OFF CACHE BOOL "disable building Google Mock" FORCE)

    # (only) Google Test 1.8.0 with BUILD_GMOCK=OFF needs BUILD_GTEST=ON
    set(BUILD_GTEST ON CACHE BOOL "enable building Google Test" FORCE)
  endif()

  # include Google Test's CMakeLists.txt
  add_subdirectory(${directory} "${CMAKE_CURRENT_BINARY_DIR}/gtest_build")
endfunction()

if (TARGET gtest)
  # try to reuse LLVM's 'gtest' target

  message(WARNING "LLVM exports its 'gtest' CMake target (for Google Test), so"
    "KLEE cannot create its own. Thus, KLEE will reuse the existing one. This"
    "is, however, only recommended if LLVM and KLEE were built using the same"
    "compiler and linker flags (to prevent compatibility issues).\n"
    "To prevent CMake from reusing the target or to use a different version "
    "of Google Test, try either of the following:\n"
    "- Point LLVM_DIR to the directory containing the `LLVMConfig.cmake` file "
    "of an installed copy of LLVM instead of a build tree.\n"
    "- Pass -DLLVM_INCLUDE_TESTS=OFF to CMake when building LLVM. This "
    "prevents building unit tests in LLVM (but not in KLEE) and exporting the "
    "target to the build tree.")

  if (GTEST_SRC_DIR)
    message(FATAL_ERROR "Cannot use GTEST_SRC_DIR when target 'gtest' is"
      "already defined.\n"
      "Either reuse LLVM's Google Test setup by not setting GTEST_SRC_DIR or "
      "choose one of the options to prevent LLVM from exporting this target.")
  endif()

  # check if it's really LLVM that exports the 'gtest' target
  list(FIND LLVM_EXPORTED_TARGETS "gtest" _GTEST_INDEX)
  if (${_GTEST_INDEX} GREATER -1)
    message(STATUS "Google Test: Reusing LLVM's 'gtest' target.")
    # in this case, only include directory has to be set
    if (LLVM_BUILD_MAIN_SRC_DIR)
      set(GTEST_INCLUDE_DIR
        "${LLVM_BUILD_MAIN_SRC_DIR}/utils/unittest/googletest/include"
        CACHE
        PATH
        "Path to Google Test include directory"
      )
    endif()
  else()
    message(FATAL_ERROR "Reusing Google Test (target) from LLVM failed:"
      "LLVM_EXPORTED_TARGETS does not contain 'gtest'.")
  endif()
else()
  # LLVM's 'gtest' target is not reused

  if (NOT GTEST_SRC_DIR)
    if (USE_CMAKE_FIND_PACKAGE_LLVM AND LLVM_BUILD_MAIN_SRC_DIR)
      # build from LLVM's utils directory
      # NOTE: This can only be done using USE_CMAKE_FIND_PACKAGE_LLVM as
      #       LLVM replaced Google Test's CMakeLists.txt with its own,
      #       requiring add_llvm_library() from AddLLVM.cmake.
      message(STATUS "Google Test: Building from LLVM's source tree.")

      set(GTEST_INCLUDE_DIR
        "${LLVM_BUILD_MAIN_SRC_DIR}/utils/unittest/googletest/include"
        CACHE
        PATH
        "Path to Google Test include directory"
      )

      add_subdirectory("${LLVM_BUILD_MAIN_SRC_DIR}/utils/unittest/"
        "${CMAKE_CURRENT_BINARY_DIR}/gtest_build")

      # add includes for LLVM's modifications
      target_include_directories(gtest BEFORE PRIVATE ${LLVM_INCLUDE_DIRS})
      # we cannot disable gtest_main, but will not use it later
      target_include_directories(gtest_main BEFORE PRIVATE ${LLVM_INCLUDE_DIRS})
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

  if (NOT TARGET gtest)
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
      message(FATAL_ERROR "Google Test source directory \"${GTEST_SRC_DIR}\" "
      "cannot be found.\n"
      "Try passing -DGTEST_SRC_DIR=<path_to_gtest_source> to CMake where "
      "<path_to_gtest_source> is the path to the Google Test source tree.\n"
      "Alternatively, you can disable unit tests by passing "
      "-DENABLE_UNIT_TESTS=OFF to CMake.")
    endif()
    message(STATUS "Google Test: Building from source.")
    message(STATUS "GTEST_SRC_DIR: ${GTEST_SRC_DIR}")

    add_vanilla_googletest_subdirectory(${GTEST_SRC_DIR})
  endif()

  # build Google Test with KLEE's defines and compile flags
  target_compile_definitions(gtest PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
  target_compile_options(gtest PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
endif()


# This keeps track of all the unit test
# targets so we can ensure they are built
# before trying to run them.
define_property(GLOBAL
  PROPERTY KLEE_UNIT_TEST_TARGETS
  BRIEF_DOCS "KLEE unit tests"
  FULL_DOCS "KLEE unit tests"
)

if (NOT IS_DIRECTORY "${GTEST_INCLUDE_DIR}")
  message(FATAL_ERROR
    "Cannot find Google Test include directory \"${GTEST_INCLUDE_DIR}\"")
endif()
message(STATUS "GTEST_INCLUDE_DIR: ${GTEST_INCLUDE_DIR}")

add_library(unittest_main)
target_sources(unittest_main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/TestMain.cpp")
klee_get_llvm_libs(UNITTEST_MAIN_LIBS Support)
target_link_libraries(unittest_main
  PUBLIC
  gtest

  PRIVATE
  ${UNITTEST_MAIN_LIBS}
)
target_include_directories(unittest_main
  PUBLIC
  ${GTEST_INCLUDE_DIR}
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
