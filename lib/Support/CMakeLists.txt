#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeSupport
  CompressionStream.cpp
  ErrorHandling.cpp
  FileHandling.cpp
  MemoryUsage.cpp
  PrintVersion.cpp
  RNG.cpp
  RoundingModeUtil.cpp
  Time.cpp
  Timer.cpp
  TreeStream.cpp
)

target_link_libraries(kleeSupport PUBLIC ${ZLIB_LIBRARIES})

set(LLVM_COMPONENTS
  support
)
klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeSupport PRIVATE ${LLVM_LIBS})

install(TARGETS kleeSupport
        EXPORT run_klee
        DESTINATION "${RUN_KLEE_LIB_DEST}"
        )
