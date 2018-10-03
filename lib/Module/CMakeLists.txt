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
  InstructionOperandTypeCheckPass.cpp
  IntrinsicCleaner.cpp
  KInstruction.cpp
  KModule.cpp
  LowerSwitch.cpp
  ModuleUtil.cpp
  Optimize.cpp
  PhiCleaner.cpp
  RaiseAsm.cpp
  Scalarizer.cpp
)

set(LLVM_COMPONENTS
  bitreader
  bitwriter
  codegen
  ipo
  irreader
  linker
  support
)

klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(kleeModule PUBLIC ${LLVM_LIBS})
target_link_libraries(kleeModule PRIVATE
  kleeSupport
)
