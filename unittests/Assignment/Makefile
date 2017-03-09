##===- unittests/Assignment/Makefile -----------------------*- Makefile -*-===##

LEVEL := ../..
include $(LEVEL)/Makefile.config

TESTNAME := Assignment
USEDLIBS := kleaverExpr.a
LINK_COMPONENTS := support

include $(LLVM_SRC_ROOT)/unittests/Makefile.unittest

CXXFLAGS += -DLLVM_29_UNITTEST
