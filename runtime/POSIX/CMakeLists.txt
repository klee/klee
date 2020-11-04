#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
# Set up
set(LIB_PREFIX "RuntimePOSIX")
set(SRC_FILES
        fd.c
        fd_32.c
        fd_64.c
        fd_init.c
        illegal.c
        klee_init_env.c
        misc.c
        selinux.c
        stubs.c
        )

# Build it
include("${CMAKE_SOURCE_DIR}/cmake/compile_bitcode_library.cmake")
prefix_with_path("${SRC_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}/" prefixed_files)
add_bitcode_library_targets("${LIB_PREFIX}" "${prefixed_files}" "-std=gnu89" "")