#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
klee_add_component(kleeModule
  Checks.cpp
  InstructionInfoTable.cpp
  IntrinsicCleaner.cpp
  KInstruction.cpp
  KModule.cpp
  LowerSwitch.cpp
  ModuleUtil.cpp
  Optimize.cpp
  PhiCleaner.cpp
  RaiseAsm.cpp
)

set(LLVM_COMPONENTS
  bitreader
  bitwriter
  codegen
  ipo
  linker
  support
)

if ("${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "3.3" OR
    "${LLVM_PACKAGE_VERSION}" VERSION_GREATER "3.3")
  list(APPEND LLVM_COMPONENTS irreader)
endif()
klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeModule PUBLIC ${LLVM_LIBS})
target_link_libraries(kleeModule PRIVATE
  kleeSupport
  # FIXME:
  # There is a circular dependency between `kleeModule` and `kleeCore`.
  # `ModuleUtil.cpp` uses `klee::SpecialFunctionHandler` (in `kleeCore`) but
  # `kleeCore` uses `kleeModule`.
  kleeCore
)
