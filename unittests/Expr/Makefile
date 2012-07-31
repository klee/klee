##===- unittests/Expr/Makefile -----------------------------*- Makefile -*-===##

LEVEL := ../..
include $(LEVEL)/Makefile.config

TESTNAME := Expr
USEDLIBS := kleaverExpr.a kleeBasic.a
USEDLIBS += $(STP_LIBS)
LINK_COMPONENTS := support

include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest

LIBS += -lstp
