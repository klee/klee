#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
function(compile_bitcode_library library_name source_files compile_cc_flags compile_cxx_flags opt_suffix)
    # Compile every source file
    set(BC_FILES)
    foreach(source_file ${source_files})
        # Get filename without extension
        get_filename_component(file_name_only "${source_file}" NAME_WE)
        set(bc_file "${CMAKE_CURRENT_BINARY_DIR}/${file_name_only}${opt_suffix}.bc" )
        get_filename_component(source_file_type "${source_file}" EXT)
        if("${source_file_type}" STREQUAL ".cpp")
            add_custom_command(
                    OUTPUT ${bc_file}
                    COMMAND ${LLVMCXX} -c "-emit-llvm" ${compile_cxx_flags} "${source_file}" -o ${bc_file}
                    DEPENDS ${source_file}
            )
        else()
            add_custom_command(
                    OUTPUT ${bc_file}
                    COMMAND ${LLVMCC} -c "-emit-llvm" ${compile_cc_flags} "${source_file}" -o ${bc_file}
                    DEPENDS ${source_file}
            )
        endif()

        list(APPEND BC_FILES ${bc_file})
    endforeach()

    # Add command to link them to an archive
    add_custom_command(
            OUTPUT ${library_name}
            COMMAND ${LLVM_AR} rcs ${library_name} ${BC_FILES}
            DEPENDS ${BC_FILES}
    )
endfunction(compile_bitcode_library)

function(prefix_with_path files prefix output_var)
    set(_result)
    foreach(file ${files})
        list(APPEND _result "${prefix}${file}")
    endforeach()
    set(${output_var} "${_result}" PARENT_SCOPE)
endfunction(prefix_with_path)

function(add_bitcode_library_targets lib_prefix prefixed_files cc_extra_args cxx_extra_args)
    set(_lib_dependencies)
    foreach(_suffix ${LIB_BC_SUFFIX})
        set(final_cc_flags ${LIB_BC_FLAGS_${_suffix}} ${cc_extra_args})
        set(final_cxx_flags ${LIB_BC_FLAGS_${_suffix}} ${cxx_extra_args})
        compile_bitcode_library("${KLEE_RUNTIME_DIRECTORY}/libklee${lib_prefix}${_suffix}.bca" "${prefixed_files}" "${final_cc_flags}" "${final_cxx_flags}" "${_suffix}")
        list(APPEND _lib_dependencies "${KLEE_RUNTIME_DIRECTORY}/libklee${lib_prefix}${_suffix}.bca")
    endforeach()

    add_custom_target(${lib_prefix} DEPENDS "${_lib_dependencies}")
endfunction(add_bitcode_library_targets)