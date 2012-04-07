##===- unittests/Solver/Makefile ---------------------------*- Makefile -*-===##

LEVEL := ../..
include $(LEVEL)/Makefile.config

TESTNAME := Solver
STP_LIBS := stp_c_interface.a stp_AST.a stp_bitvec.a \
            stp_constantbv.a stp_sat.a stp_simplifier.a
USEDLIBS := kleaverSolver.a kleaverExpr.a kleeSupport.a kleeBasic.a
ifeq ($(ENABLE_EXT_STP),0)
  USEDLIBS += $(STP_LIBS)
endif
LINK_COMPONENTS := support

include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest

ifeq ($(ENABLE_EXT_STP),1)
  LIBS += -lstp
endif
