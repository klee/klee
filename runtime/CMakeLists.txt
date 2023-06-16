#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
# Handle binaries
add_subdirectory(Runtest)

# Handle bitcode libraries
# Define the different configurations to be compiled and made available using a specific suffix
set(bc_architectures 64)

# Compile 32bit support if available
if (M32_SUPPORTED)
    list(APPEND bc_architectures "32")
endif ()

set(LIB_BC_SUFFIX "")

foreach (bc_architecture ${bc_architectures})
    foreach (bc_optimization ${available_klee_runtime_build_types})
        # Add configuration to the set of available configurations
        list(APPEND LIB_BC_SUFFIX "${bc_architecture}_${bc_optimization}")

        # Set specific flags for that configuration
        set(local_flags "")
        if (bc_architecture EQUAL "32")
            list(APPEND local_flags -m32)
        endif ()


        # Set specific compiler flags depending on the optimization
        if (bc_optimization STREQUAL "Release")
            list(APPEND local_flags -O2 -DNDEBUG)
        elseif (bc_optimization STREQUAL "Release+Debug")
            list(APPEND local_flags -O2 -g -D_DEBUG -DNDEBUG)
        elseif (bc_optimization STREQUAL "Release+Asserts")
            list(APPEND local_flags -O2)
        elseif (bc_optimization STREQUAL "Release+Debug+Asserts")
            list(APPEND local_flags -O2 -g -D_DEBUG)
        elseif (bc_optimization STREQUAL "Debug")
            list(APPEND local_flags -g -D_DEBUG -fpermissive)
        elseif (bc_optimization STREQUAL "Debug+Asserts")
            list(APPEND local_flags -g -D_DEBUG)
        else()
            message(FATAL_ERROR
                    "Optimization (\"${bc_optimization}\") for runtime library unknown.")
        endif ()

        # Define suffix-specific optimizations
        set("LIB_BC_FLAGS_${bc_architecture}_${bc_optimization}" ${local_flags})
    endforeach ()
endforeach ()

message(STATUS "LIB_BC_SUFFIX: ${LIB_BC_SUFFIX}")
message(STATUS "KLEE_RUNTIME_DIRECTORY: ${KLEE_RUNTIME_DIRECTORY}")

# Add additional setups if needed, e.g.
# `list(APPEND LIB_BC_SUFFIX MY_SPECIAL_CONFIG)`

# Following define the specific flags: LIB_BC_FLAGS_*SUFFIX_FROM_ABOVE*
set(LIB_BC_FLAGS_64)
set(LIB_BC_FLAGS_32 -m32)

# Common for all library configurations
# Since the runtime now contains fortified libc functions, it is
# important to compile it with -D_FORTIFY_SOURCE=0 to avoid infinite
# recursion
set(COMMON_CC_FLAGS
        "-I${CMAKE_SOURCE_DIR}/include"
        "-I${CMAKE_BINARY_DIR}/include"
        -D_FORTIFY_SOURCE=0
        -D_GNU_SOURCE
        -D__STDC_LIMIT_MACROS
        -D__STDC_CONSTANT_MACROS
        -Wall
        -Wwrite-strings
        -Xclang
        -disable-O0-optnone
        )

foreach (_suffix ${LIB_BC_SUFFIX})
    list(APPEND "LIB_BC_FLAGS_${_suffix}" ${COMMON_CC_FLAGS})
endforeach ()

add_subdirectory(Freestanding)
add_subdirectory(Intrinsic)
add_subdirectory(klee-libc)
add_subdirectory(Fortify)
add_subdirectory(Mocks)

set(RUNTIME_LIBRARIES
        RuntimeFreestanding
        RuntimeIntrinsic
        RuntimeKLEELibc
        RuntimeFortify
        RuntimeMocks
        )

if (ENABLE_KLEE_EH_CXX)
    list(APPEND RUNTIME_LIBRARIES eh-cxx)
    add_subdirectory(klee-eh-cxx)
endif ()

if (ENABLE_POSIX_RUNTIME)
    list(APPEND RUNTIME_LIBRARIES "RuntimePOSIX;RuntimeIO_C")
    add_subdirectory(POSIX)
endif ()

if (ENABLE_FP_RUNTIME)
    list(APPEND RUNTIME_LIBRARIES RuntimeFp)
    add_subdirectory(klee-fp)
endif ()

list(APPEND RUNTIME_LIBRARIES UBSan)
add_subdirectory(Sanitizer)

add_custom_target(BuildKLEERuntimes
        DEPENDS "${RUNTIME_LIBRARIES}"
        )

install(DIRECTORY "${KLEE_RUNTIME_DIRECTORY}/"
        DESTINATION "${KLEE_INSTALL_RUNTIME_DIR}"
        )
