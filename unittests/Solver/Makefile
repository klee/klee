##===- unittests/Solver/Makefile ---------------------------*- Makefile -*-===##

LEVEL := ../..
TESTNAME := Solver
STP_LIBS := stp_c_interface.a stp_AST.a stp_bitvec.a \
            stp_constantbv.a stp_sat.a stp_simplifier.a
USEDLIBS := kleaverSolver.a kleaverExpr.a kleeSupport.a kleeBasic.a $(STP_LIBS)
LINK_COMPONENTS := support

include $(LEVEL)/Makefile.config
include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest

#LIBS += -lstp 
