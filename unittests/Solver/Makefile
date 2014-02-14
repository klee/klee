##===- unittests/Solver/Makefile ---------------------------*- Makefile -*-===##

LEVEL := ../..
include $(LEVEL)/Makefile.config

TESTNAME := Solver
USEDLIBS := kleaverSolver.a kleaverExpr.a kleeSupport.a kleeBasic.a
LINK_COMPONENTS := support

include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest

LIBS += -lstp

ifeq ($(STP_NEEDS_BOOST),1)
	LIBS += $(UPSTREAM_STP_LINK_FLAGS)
endif
