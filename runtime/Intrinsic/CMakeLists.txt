#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
# Set up
set(LIB_PREFIX "RuntimeIntrinsic")
set(SRC_FILES
        dso_handle.c
        klee_choose.c
        klee_div_zero_check.c
        klee_int.c
        klee_is_replay.c
        klee_overshift_check.c
        klee_range.c
        )

# Build it
include("${CMAKE_SOURCE_DIR}/cmake/compile_bitcode_library.cmake")
prefix_with_path("${SRC_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}/" prefixed_files)
add_bitcode_library_targets("${LIB_PREFIX}" "${prefixed_files}" "-std=gnu89" "")