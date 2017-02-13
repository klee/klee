#===- test/Makefile ----------------------------------------*- Makefile -*--===#
# 
#                     The LLVM Compiler Infrastructure
#
# This file was developed by the LLVM research group and is distributed under
# the University of Illinois Open Source License. See LICENSE.TXT for details.
# 
#===------------------------------------------------------------------------===#

LEVEL  = ..
DIRS   = 

#
# Make llvm-lit the default for testing
#
all:: systemtests-local

# Include other test rules
include Makefile.tests

#===------------------------------------------------------------------------===#
# llvm-lit testing support
#===------------------------------------------------------------------------===#
ULIMIT = ulimit -t 600 ; ulimit -d 512000 ;

ifdef VERBOSE
LIT_ARGS := -v
else
LIT_ARGS := -s -v
endif

ifdef TESTSUITE
LIT_TESTSUITE := $(TESTSUITE)
else
LIT_TESTSUITE := .
endif

# Older LLVM versions don't detect python at configure time
# so guess the location
ifndef PYTHON
        # llvm-lit doesn't work with python3 so force python2
	PYTHON := python2
endif

# Potential support in the future for multiple test suites
LIT_ALL_TESTSUITES := $(LIT_TESTSUITE)

systemtests-local:: lit.site.cfg
	$(Verb) ( $(ULIMIT) \
	          $(PYTHON) $(LLVM_SRC_ROOT)/utils/lit/lit.py $(LIT_ARGS) $(LIT_ALL_TESTSUITES) )

clean::
	$(RM) -rf `find $(PROJ_OBJ_ROOT)/test -name Output -type d -print`
	$(RM) -rf `find $(PROJ_OBJ_ROOT)/test -name 'ft-out*' -type d -print`
	$(RM) -rf `find $(PROJ_OBJ_ROOT)/test -name 'ft-last' -print`
	$(RM) -rf `find $(PROJ_OBJ_ROOT)/test -name 'klee-last'`
	$(RM) -rf `find $(PROJ_OBJ_ROOT)/test -name 'klee-out*'`
	$(RM) -rf `find $(PROJ_OBJ_ROOT)/test -name '*~'`
	$(RM) -rf `find $(PROJ_OBJ_ROOT)/test -name test.log`

lit.site.cfg: lit.site.cfg.in
	@echo "Making KLEE 'lit.site.cfg' file..."
	@sed -e "s#@KLEE_SOURCE_DIR@#$(PROJ_SRC_ROOT)#g" \
	     -e "s#@KLEE_BINARY_DIR@#$(PROJ_OBJ_ROOT)#g" \
	     -e "s#@KLEE_TOOLS_DIR@#$(ToolDir)#g" \
	     -e "s#@LLVM_TOOLS_DIR@#$(LLVMToolDir)#g" \
	     -e "s#@LLVM_VERSION_MAJOR@#$(LLVM_VERSION_MAJOR)#g" \
	     -e "s#@LLVM_VERSION_MINOR@#$(LLVM_VERSION_MINOR)#g" \
	     -e "s#@LLVMCC@#$(KLEE_BITCODE_C_COMPILER) -I$(PROJ_SRC_ROOT)/include#g" \
	     -e "s#@LLVMCXX@#$(KLEE_BITCODE_CXX_COMPILER) -I$(PROJ_SRC_ROOT)/include#g" \
	     -e "s#@ENABLE_UCLIBC@#$(ENABLE_UCLIBC)#g" \
	     -e "s#@ENABLE_POSIX_RUNTIME@#$(ENABLE_POSIX_RUNTIME)#g" \
	     -e "s#@TARGET_TRIPLE@#$(TARGET_TRIPLE)#g" \
	     -e "s#@HAVE_SELINUX@#$(HAVE_SELINUX)#g" \
	     -e "s#@ENABLE_STP@#$(ENABLE_STP)#g" \
	     -e "s#@ENABLE_Z3@#$(ENABLE_Z3)#g" \
	     -e "s#@NATIVE_CC@#$(CC) $(CFLAGS) -I$(PROJ_SRC_ROOT)/include#g" \
	     -e "s#@NATIVE_CXX@#$(CXX) $(CXXFLAGS) -I$(PROJ_SRC_ROOT)/include#g" \
	     -e "s#@LIB_KLEE_RUN_TEST_PATH@#$(SharedLibDir)/$(SharedPrefix)kleeRuntest$(SHLIBEXT)#g" \
	     $(PROJ_SRC_DIR)/lit.site.cfg.in > $@
