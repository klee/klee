#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(kleeSupport
  CompressionStream.cpp
  ErrorHandling.cpp
  FileHandling.cpp
  MemoryUsage.cpp
  PrintVersion.cpp
  RNG.cpp
  Time.cpp
  Timer.cpp
  TreeStream.cpp
)

llvm_config(kleeSupport "${USE_LLVM_SHARED}" support)

target_link_libraries(kleeSupport PRIVATE ${ZLIB_LIBRARIES} ${TCMALLOC_LIBRARIES})
target_include_directories(kleeSupport PRIVATE ${KLEE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS} ${TCMALLOC_INCLUDE_DIR})
target_compile_options(kleeSupport PRIVATE ${KLEE_COMPONENT_CXX_FLAGS})
target_compile_definitions(kleeSupport PRIVATE ${KLEE_COMPONENT_CXX_DEFINES})
