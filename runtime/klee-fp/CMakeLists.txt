#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

set(LIB_PREFIX "RuntimeFp")
set(SRC_FILES
    ceil.c
    klee_copysign.c
    exp.c
    fabs.c
    klee_fenv.c
    klee_floor.c
    fpclassify.c
    klee_internal_isinf.ll
    klee_rint.c
    klee_set_rounding_mode.c
    klee_signbit.ll
    log.c
    round.c
    sqrt.c
    trigonometry.c
    )

# Add __NO_INLINE__ to prevent glibc from using inline definitions of some
# builtins.
set(ADDITIONAL_CC_FLAGS
        -D__NO_INLINE__
        -std=gnu89
        )

# Build it
include("${CMAKE_SOURCE_DIR}/cmake/compile_bitcode_library.cmake")
prefix_with_path("${SRC_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}/" prefixed_files)

add_bitcode_library_targets("${LIB_PREFIX}" "${prefixed_files}" "${ADDITIONAL_CC_FLAGS}" "")
