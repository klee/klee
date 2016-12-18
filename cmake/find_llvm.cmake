#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
#
# This file provides multiple methods to detect LLVM.
#
# * llvm-config executable. This method is portable across LLVM build systems
# (i.e. works if LLVM was built with autoconf/Makefile or with CMake).
#
# * find_package(LLVM CONFIG). This method only works if LLVM was built with
# CMake or with LLVM >= 3.5 when built with the autoconf/Makefile build system
# This method relies on the `LLVMConfig.cmake` file generated to be generated
# by LLVM's build system.
#
#===------------------------------------------------------------------------===#

option(USE_CMAKE_FIND_PACKAGE_LLVM "Use find_package(LLVM CONFIG) to find LLVM" OFF)

if (USE_CMAKE_FIND_PACKAGE_LLVM)
  find_package(LLVM CONFIG REQUIRED)

  # Provide function to map LLVM components to libraries.
  function(klee_get_llvm_libs output_var)
    if (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.5")
      llvm_map_components_to_libraries(${output_var} ${ARGN})
    else()
      llvm_map_components_to_libnames(${output_var} ${ARGN})
    endif()
    set(${output_var} ${${output_var}} PARENT_SCOPE)
  endfunction()
else()
  # Use the llvm-config binary to get the information needed.
  # Try to detect it in the user's environment. The user can
  # force a particular binary by passing `-DLLVM_CONFIG_BINARY=/path/to/llvm-config`
  # to CMake.
  find_program(LLVM_CONFIG_BINARY
    NAMES llvm-config)
  message(STATUS "LLVM_CONFIG_BINARY: ${LLVM_CONFIG_BINARY}")

  if (NOT LLVM_CONFIG_BINARY)
    message(FATAL_ERROR
      "Failed to find llvm-config.\n"
      "Try passing -DLLVM_CONFIG_BINARY=/path/to/llvm-config to cmake")
  endif()

  function(_run_llvm_config output_var)
    set(_command "${LLVM_CONFIG_BINARY}" ${ARGN})
    execute_process(COMMAND ${_command}
      RESULT_VARIABLE _exit_code
      OUTPUT_VARIABLE ${output_var}
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
    )
    if (NOT ("${_exit_code}" EQUAL "0"))
      message(FATAL_ERROR "Failed running ${_command}")
    endif()
    set(${output_var} ${${output_var}} PARENT_SCOPE)
  endfunction()

  # Get LLVM version
  _run_llvm_config(LLVM_PACKAGE_VERSION "--version")
  # Try x.y.z patern
  set(_llvm_version_regex "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(svn)?$")
  if ("${LLVM_PACKAGE_VERSION}" MATCHES "${_llvm_version_regex}")
    string(REGEX REPLACE
      "${_llvm_version_regex}"
      "\\1"
      LLVM_VERSION_MAJOR
      "${LLVM_PACKAGE_VERSION}")
    string(REGEX REPLACE
      "${_llvm_version_regex}"
      "\\2"
      LLVM_VERSION_MINOR
      "${LLVM_PACKAGE_VERSION}")
    string(REGEX REPLACE
      "${_llvm_version_regex}"
      "\\3"
      LLVM_VERSION_PATCH
      "${LLVM_PACKAGE_VERSION}")
  else()
    # try x.y pattern
    set(_llvm_version_regex "^([0-9]+)\\.([0-9]+)$")
    if ("${LLVM_PACKAGE_VERSION}" MATCHES "${_llvm_version_regex}")
      string(REGEX REPLACE
        "${_llvm_version_regex}"
        "\\1"
        LLVM_VERSION_MAJOR
        "${LLVM_PACKAGE_VERSION}")
    string(REGEX REPLACE
      "${_llvm_version_regex}"
      "\\2"
      LLVM_VERSION_MINOR
      "${LLVM_PACKAGE_VERSION}")
    set(LLVM_VERSION_PATCH 0)
    else()
      message(FATAL_ERROR
        "Failed to parse LLVM version from \"${LLVM_PACKAGE_VERSION}\"")
    endif()
  endif()

  set(LLVM_DEFINITIONS "")
  _run_llvm_config(_llvm_cpp_flags "--cppflags")
  string_to_list("${_llvm_cpp_flags}" _llvm_cpp_flags_list)
  foreach (flag ${_llvm_cpp_flags_list})
    # Filter out -I flags by only looking for -D flags.
    if ("${flag}" MATCHES "^-D" AND NOT ("${flag}" STREQUAL "-D_DEBUG"))
      list(APPEND LLVM_DEFINITIONS "${flag}")
    endif()
  endforeach()

  set(LLVM_ENABLE_ASSERTIONS ON)
  set(LLVM_ENABLE_EH ON)
  set(LLVM_ENABLE_RTTI ON)
  _run_llvm_config(_llvm_cxx_flags "--cxxflags")
  string_to_list("${_llvm_cxx_flags}" _llvm_cxx_flags_list)
  foreach (flag ${_llvm_cxx_flags_list})
    if ("${flag}" STREQUAL "-DNDEBUG")
      # Note we don't rely on `llvm-config --build-mode` because
      # that seems broken when LLVM is built with CMake.
      set(LLVM_ENABLE_ASSERTIONS OFF)
    elseif ("${flag}" STREQUAL "-fno-exceptions")
      set(LLVM_ENABLE_EH OFF)
    elseif ("${flag}" STREQUAL "-fno-rtti")
      set(LLVM_ENABLE_RTTI OFF)
    endif()
  endforeach()

  set(LLVM_INCLUDE_DIRS "")
  foreach (flag ${_llvm_cpp_flags_list})
    # Filter out -D flags by only looking for -I flags.
    if ("${flag}" MATCHES "^-I")
      string(REGEX REPLACE "^-I(.+)$" "\\1" _include_dir "${flag}")
      list(APPEND LLVM_INCLUDE_DIRS "${_include_dir}")
    endif()
  endforeach()

  _run_llvm_config(LLVM_LIBRARY_DIRS "--libdir")
  _run_llvm_config(LLVM_TOOLS_BINARY_DIR "--bindir")
  _run_llvm_config(TARGET_TRIPLE "--host-target")

  # Provide function to map LLVM components to libraries.
  function(klee_get_llvm_libs OUTPUT_VAR)
    _run_llvm_config(_llvm_libs "--libfiles" ${ARGN})
    string_to_list("${_llvm_libs}" _llvm_libs_list)

    # Now find the system libs that are needed.
    if (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.5")
      # For LLVM 3.4 and older system libraries
      # appeared in the output of `--ldflags`.
      _run_llvm_config(_system_libs "--ldflags")
      # TODO: Filter out `-L<path>` flag.
    else()
      _run_llvm_config(_system_libs "--system-libs")
    endif()
    string_to_list("${_system_libs}" _system_libs_list)

    # Create an imported target for each LLVM library
    # if it doesn't already exist. We need to do this
    # so we can tell CMake that these libraries depend
    # on the necessary libraries so that CMake
    # can get the link order right.
    set(targets_to_return "")
    set(created_targets "")
    foreach (llvm_lib ${_llvm_libs_list})
      get_filename_component(llvm_lib_file_name "${llvm_lib}" NAME)
      string(REGEX REPLACE "^(lib)?(LLVM[a-zA-Z0-9]+)\\..+$" "\\2" target_name "${llvm_lib_file_name}")
      list(APPEND targets_to_return "${target_name}")
      if (NOT TARGET "${target_name}")
        # DEBUG: message(STATUS "Creating imported target \"${target_name}\"" " for \"${llvm_lib}\"")
        list(APPEND created_targets "${target_name}")

        set(import_library_type "STATIC")
        if ("${llvm_lib_file_name}" MATCHES "(so|dylib|dll)$")
          set(import_library_type "SHARED")
        endif()
        # Create an imported target for the library
        add_library("${target_name}" "${import_library_type}" IMPORTED GLOBAL)
        set_property(TARGET "${target_name}" PROPERTY
          IMPORTED_LOCATION "${llvm_lib}"
        )
      endif()
    endforeach()

    # Now state the dependencies of the created imported targets which we
    # assume to be for each imported target the libraries which appear after
    # the library in `{_llvm_libs_list}` and then finally the system libs.
    # It is **essential** that we do this otherwise CMake will get the
    # link order of the imported targets wrong.
    list(LENGTH targets_to_return length_targets_to_return)
    if ("${length_targets_to_return}" GREATER 0)
      math(EXPR targets_to_return_last_index "${length_targets_to_return} -1")
      foreach (llvm_target_lib ${created_targets})
        # DEBUG: message(STATUS "Adding deps for target ${llvm_target_lib}")
        # Find position in `targets_to_return`
        list(FIND targets_to_return "${llvm_target_lib}" position)
        if ("${position}" EQUAL "-1")
          message(FATAL_ERROR "couldn't find \"${llvm_target_lib}\" in list of targets")
        endif()
        if ("${position}" LESS "${targets_to_return_last_index}")
          math(EXPR position_plus_one "${position} + 1")
          foreach (index RANGE ${position_plus_one} ${targets_to_return_last_index})
            # Get the target for this index
            list(GET targets_to_return ${index} target_for_index)
            # DEBUG: message(STATUS "${llvm_target_libs} depends on ${target_for_index}")
            set_property(TARGET "${llvm_target_lib}" APPEND PROPERTY
              INTERFACE_LINK_LIBRARIES "${target_for_index}"
            )
          endforeach()
        endif()
        # Now finally add the system library dependencies. These must be last.
        set_property(TARGET "${target_name}" APPEND PROPERTY
          INTERFACE_LINK_LIBRARIES "${_system_libs_list}"
        )
      endforeach()
    endif()

    set(${OUTPUT_VAR} ${targets_to_return} PARENT_SCOPE)
  endfunction()
endif()
