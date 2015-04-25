#===-- runtime/klee-uclibc/Makefile --------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

# Note klee-uclibc does not really live here.  This makefile just manages the
# location of the klee-uclibc bitcode archive when building and when installing
# KLEE.

LEVEL=../..

# We're not really building a bytecode library here
# but we need to set this so that $(BuildMode) is
# set appropriately
BYTECODE_LIBRARY=1

include $(LEVEL)/Makefile.common

# The purpose of setting up this symbolic link is
# so that KLEE can always look for klee-uclibc
# in the same place it looks for all the other
# run time libraries
uclibc_symlink:=$(PROJ_OBJ_ROOT)/$(BuildMode)/lib/$(KLEE_UCLIBC_BCA_NAME)
#
# Force our extra rules to run
all-local:: $(uclibc_symlink)

$(uclibc_symlink):
	@echo "Setting up symbolic link to klee-uclibc"
	-$(Verb) $(MKDIR) $(PROJ_OBJ_ROOT)/$(BuildMode)/lib
	$(Verb) ln -s -f $(KLEE_UCLIBC_BCA) $(uclibc_symlink)

# The reasons for copying over klee-uclibc on install are
#
# * KLEE can look for klee-uclibc in the same place it looks for all other run
#   time libraries. 
# * KLEE can be more easily distributed with klee-uclibc

install:: copy_klee_uclibc
uninstall:: remove_klee_uclibc

.PHONY: copy_klee_uclibc remove_klee_uclibc

copy_klee_uclibc:
	@echo "Installing klee-uclibc archive"
	$(Verb) $(CP) $(KLEE_UCLIBC_BCA) $(BYTECODE_DESTINATION)/$(KLEE_UCLIBC_BCA_NAME)

remove_klee_uclibc:
	@echo "Removing klee-uclibc archive"
	$(Verb) $(RM) $(BYTECODE_DESTINATION)/$(KLEE_UCLIBC_BCA_NAME)
