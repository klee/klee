#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

function(klee_add_component target_name)
  # Components are explicitly STATIC because we don't support building them
  # as shared libraries.
  add_library(${target_name} STATIC ${ARGN})
  # Use of `PUBLIC` means these will propagate to targets that use this component.
  if (("${CMAKE_VERSION}" VERSION_EQUAL "3.3") OR ("${CMAKE_VERSION}" VERSION_GREATER "3.3"))
    # In newer CMakes we can make sure that the flags are only used when compiling C++
    target_compile_options(${target_name} PUBLIC
      $<$<COMPILE_LANGUAGE:CXX>:${KLEE_COMPONENT_CXX_FLAGS}>)
  else()
    # For older CMakes just live with the warnings we get for passing C++ only flags
    # to the C compiler.
    target_compile_options(${target_name} PUBLIC ${KLEE_COMPONENT_CXX_FLAGS})
  endif()
  target_include_directories(${target_name} PUBLIC ${KLEE_COMPONENT_EXTRA_INCLUDE_DIRS})
  target_compile_definitions(${target_name} PUBLIC ${KLEE_COMPONENT_CXX_DEFINES})
  target_link_libraries(${target_name} PUBLIC ${KLEE_COMPONENT_EXTRA_LIBRARIES})
endfunction()
