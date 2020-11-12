#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

set(LIB_PREFIX "RuntimeFortify")
set(SRC_FILES
        fortify.c
        ../Freestanding/fortify-fs.c
        ../klee-libc/fortify-klibc.c
        )

# Add __NO_INLINE__ to prevent glibc from using inline definitions of some
# builtins (i.e. putchar and atoi).
set(ADDITIONAL_CC_FLAGS
        -D__NO_INLINE__
        -std=gnu89
        )

# Build it
include("${CMAKE_SOURCE_DIR}/cmake/compile_bitcode_library.cmake")
prefix_with_path("${SRC_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}/" prefixed_files)

add_bitcode_library_targets("${LIB_PREFIX}" "${prefixed_files}" "${ADDITIONAL_CC_FLAGS}" "")
