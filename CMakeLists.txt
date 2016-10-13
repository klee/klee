cmake_minimum_required(VERSION 3.0)

project(KLEE VERSION 1.2.0)
set(PACKAGE_STRING "\"KLEE ${KLEE_VERSION}\"")
set(PACKAGE_URL "\"https://klee.github.io\"")
set(KLEE_CMAKE_BUILD 1)
set(RUNTIME_CONFIGURATION "\"\"")

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckPrototypeDefinition)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions(-D_GNU_SOURCE
	-DKLEE_DIR=\"${CMAKE_BINARY_DIR}\"
	-DKLEE_INSTALL_BIN_DIR=\"${CMAKE_INSTALL_FULL_BINDIR}\"
	-DKLEE_INSTALL_RUNTIME_DIR=\"${CMAKE_INSTALL_FULL_LIBDIR}/klee/runtime\")

find_package(ZLIB REQUIRED)
if (${ZLIB_FOUND})
	set(HAVE_ZLIB_H 1)
	set(TARGET_LIBS ${TARGET_LIBS} z)
endif()

find_library(STP stp)
if (${STP} STREQUAL "STP-NOTFOUND")
	message(STATUS "STP NOT found")
else()
	check_include_file_cxx(stp/c_interface.h STP_INCLUDE)
	message(STATUS "Found STP: ${STP}")
	set(ENABLE_STP 1)
	set(TARGET_LIBS ${TARGET_LIBS} stp)
endif()

find_library(Z3 z3)
if (${Z3} STREQUAL "Z3-NOTFOUND")
	message(STATUS "Z3 NOT found")
else()
	check_include_file_cxx(z3.h Z3_INCLUDE)
	check_prototype_definition(Z3_get_error_msg
		"Z3_string Z3_get_error_msg(Z3_context c, Z3_error_code err)"
		"NULL" "z3.h" HAVE_Z3_GET_ERROR_MSG_NEEDS_CONTEXT)
	message(STATUS "Found Z3: ${Z3}")
	set(ENABLE_Z3 1)
	set(TARGET_LIBS ${TARGET_LIBS} z3)
endif()

if (NOT 0${ENABLE_Z3} AND NOT 0${ENABLE_STP})
	message(FATAL_ERROR "Neither Z3 nor STP found. We need at least one solver!")
endif()

check_include_file_cxx(gperftools/malloc_extension.h HAVE_GPERFTOOLS_MALLOC_EXTENSION_H)
if (${HAVE_GPERFTOOLS_MALLOC_EXTENSION_H})
	set(TARGET_LIBS ${TARGET_LIBS} tcmalloc_minimal)
endif()

check_include_file(sys/capability.h HAVE_SYS_CAPABILITY_H)

check_function_exists(mallinfo HAVE_MALLINFO)

check_function_exists(__ctype_b_loc HAVE_CTYPE_EXTERNALS)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/klee/Config/config.h.cmin
	${CMAKE_CURRENT_BINARY_DIR}/include/klee/Config/config.h)

include_directories(include)
include_directories(${CMAKE_BINARY_DIR}/include)

macro(klee_build_llvm_library lib)
	foreach(in_f ${ARGN})
		get_filename_component(out_f ${in_f} NAME)
		set(out_f ${out_f}.bc)
		add_custom_command(OUTPUT ${out_f}
			COMMAND ${CLANG_PATH} -emit-llvm -c -o ${out_f} ${CMAKE_CURRENT_SOURCE_DIR}/${in_f}
			DEPENDS ${in_f}
			COMMENT "Building LLVM bitcode ${out_f}"
			VERBATIM
		)
		list(APPEND result ${out_f})
	endforeach()
	add_custom_command(OUTPUT ${lib}.bc
		COMMAND ${LLVM_LINK_PATH} -o ${lib}.bc ${result}
		DEPENDS ${result}
		COMMENT "Linking LLVM bitcode ${lib}.bc"
		VERBATIM
	)
	add_custom_target(${lib} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${lib}.bc)
endmacro()

macro(klee_llvm_libnames llvm_libs)
	if (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.5")
		llvm_map_components_to_libraries(${llvm_libs} ${ARGN})
	else()
		llvm_map_components_to_libnames(${llvm_libs} ${ARGN})
	endif()
endmacro()

add_subdirectory(lib)
add_subdirectory(runtime)
add_subdirectory(tools)
#add_subdirectory(test)

install(FILES include/klee/klee.h DESTINATION include)
