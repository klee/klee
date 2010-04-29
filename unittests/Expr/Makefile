##===- unittests/Expr/Makefile -----------------------------*- Makefile -*-===##

LEVEL := ../..
TESTNAME := Expr
STP_LIBS := stp_c_interface.a stp_AST.a stp_bitvec.a \
            stp_constantbv.a stp_sat.a stp_simplifier.a
USEDLIBS := kleaverExpr.a kleeBasic.a $(STP_LIBS)
LINK_COMPONENTS := support

include $(LEVEL)/Makefile.config
include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest


