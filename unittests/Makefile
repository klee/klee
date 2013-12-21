##===- unittests/Makefile ----------------------------------*- Makefile -*-===##

LEVEL = ..

include $(LEVEL)/Makefile.config

LIBRARYNAME = UnitTestMain
BUILD_ARCHIVE = 1
CPP.Flags += -I$(LLVM_SRC_ROOT)/utils/unittest/googletest/include/
CPP.Flags += -Wno-variadic-macros

# FIXME: Parallel dirs is broken?
DIRS = Expr Solver Ref

include $(LEVEL)/Makefile.common

# Remove -fno-rtti as this prevents typeid() being used
# in gtest
CXX.Flags := $(filter-out -fno-rtti,$(CXX.Flags))

clean::
	$(Verb) $(RM) -f *Tests
