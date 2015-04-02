##===- unittests/Expr/Makefile -----------------------------*- Makefile -*-===##

LEVEL := ../..
include $(LEVEL)/Makefile.config

TESTNAME := Expr
USEDLIBS := kleaverExpr.a kleeBasic.a
LINK_COMPONENTS := support

include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest

CXXFLAGS += -DLLVM_29_UNITTEST
LIBS += $(STP_LDFLAGS)
