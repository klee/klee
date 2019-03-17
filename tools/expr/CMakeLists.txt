#===------------------------------------------------------------------------===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#
add_library(expr SHARED
  expr.cpp
  ../../lib/Expr/ArrayCache.cpp
  ../../lib/Expr/ExprBuilder.cpp
  ../../lib/Expr/ExprPPrinter.cpp
  ../../lib/Expr/Expr.cpp
  ../../lib/Expr/Updates.cpp
)

target_link_options(expr PUBLIC "LINKER:--no-undefined")
if (NOT LLVM_ENABLE_RTTI)
  if (ENABLE_SOLVER_METASMT AND metaSMT_REQUIRE_RTTI)
    message(FATAL_ERROR
      "RTTI cannot be disabled because metaSMT uses them."
      "This build configuration is not supported and will likely not work."
      "You should recompile LLVM with RTTI enabled.")
  else()
    target_compile_options(expr PUBLIC "-fno-rtti")
  endif()
endif()

set(LLVM_COMPONENTS
  support
)
klee_get_llvm_libs(LLVM_LIBS ${LLVM_COMPONENTS})
target_link_libraries(expr PUBLIC ${LLVM_LIBS})
