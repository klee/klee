#===-- klee/Makefile ---------------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

#
# Indicates our relative path to the top of the project's root directory.
#
LEVEL = .

include $(LEVEL)/Makefile.config

# The header files are normally installed
# by the install-local target in the top-level
# makefile. This disables installing anything
# in the top-level makefile.
NO_INSTALL=1

DIRS = lib tools runtime
EXTRA_DIST = include

# Only build support directories when building unittests.
ifeq ($(MAKECMDGOALS),unittests)
  DIRS := $(filter-out tools runtime, $(DIRS)) unittests
  OPTIONAL_DIRS :=
endif

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common

.PHONY: doxygen
doxygen:
	doxygen docs/doxygen.cfg

.PHONY: cscope.files
cscope.files:
	find \
          lib include tools runtime examples unittests test \
          -name Makefile -or \
          -name \*.in -or \
          -name \*.c -or \
          -name \*.cpp -or \
          -name \*.exp -or \
          -name \*.inc -or \
          -name \*.h | sort > cscope.files

test::
	-(cd test/ && make)

.PHONY: klee-cov
klee-cov:
	rm -rf klee-cov
	zcov-scan --look-up-dirs=1 klee.zcov .
	zcov-genhtml --root $$(pwd) klee.zcov klee-cov

clean::
	$(MAKE) -C test clean 
	$(MAKE) -C unittests clean
	rm -rf docs/doxygen test/lit.site.cfg

# Install klee instrinsic header file
install-local::
	$(MKDIR) $(DESTDIR)$(PROJ_includedir)/klee
	$(DataInstall) $(PROJ_SRC_ROOT)/include/klee/klee.h $(DESTDIR)$(PROJ_includedir)/klee/klee.h
