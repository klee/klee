#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

set(LIB_PREFIX "eh-cxx")
set(SRC_FILES
        klee_eh_cxx.cpp
        )

set(ADDITIONAL_CXX_FLAGS
        -nostdinc++
        -I "${KLEE_LIBCXXABI_SRC_DIR}/src"
        -I "${KLEE_LIBCXXABI_SRC_DIR}/include"
        -I "${KLEE_LIBCXX_INCLUDE_DIR}"
        )
# Build it
include("${CMAKE_SOURCE_DIR}/cmake/compile_bitcode_library.cmake")
prefix_with_path("${SRC_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}/" prefixed_files)
add_bitcode_library_targets("${LIB_PREFIX}" "${prefixed_files}" "" "${ADDITIONAL_CXX_FLAGS}")