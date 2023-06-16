#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

set(LIB_PREFIX "RuntimeMocks")
set(SRC_FILES
        models.c
        )

# Build it
include("${CMAKE_SOURCE_DIR}/cmake/compile_bitcode_library.cmake")
prefix_with_path("${SRC_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}/" runtime_mocks_files)

add_bitcode_library_targets("${LIB_PREFIX}" "${runtime_mocks_files}" "" "")
