#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
set(KLEE_MODULE_COMPONENT_SRCS
  CallSplitter.cpp
  Checks.cpp
  CodeGraphDistance.cpp
  FunctionAlias.cpp
  InstructionInfoTable.cpp
  InstructionOperandTypeCheckPass.cpp
  IntrinsicCleaner.cpp
  KInstruction.cpp
  KModule.cpp
  KType.cpp
  LowerSwitch.cpp
  ModuleUtil.cpp
  Optimize.cpp
  OptNone.cpp
  PhiCleaner.cpp
  RaiseAsm.cpp
  ReturnSplitter.cpp
)

if (USE_WORKAROUND_LLVM_PR39177)
  list(APPEND KLEE_MODULE_COMPONENT_SRCS WorkaroundLLVMPR39177.cpp)
endif()

klee_add_component(kleeModule
  ${KLEE_MODULE_COMPONENT_SRCS}
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
target_link_libraries(kleeModule PRIVATE ${LLVM_LIBS})
target_link_libraries(kleeModule PUBLIC
  kleeSupport
)

install(TARGETS kleeModule
        EXPORT run_klee
        DESTINATION "${RUN_KLEE_LIB_DEST}"
        )
