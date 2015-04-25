#===-- lib/Support/Makefile --------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..

LIBRARYNAME=kleeSupport
DONT_BUILD_RELINKED=1
BUILD_ARCHIVE=1
NO_INSTALL=1

# FIXME: This is nasty. We don't want to rebuild this library everytime
# but this was the only way I could find to make the build work.
#
# Note this rule has to go here so it is run first.
CompileTimeInfoFile:=../../include/klee/Config/CompileTimeInfo.h
all-local:: $(CompileTimeInfoFile)

include $(LEVEL)/Makefile.common


GIT_PRESENT:=$(shell [ -d "$(PROJ_SRC_ROOT)/.git" ] && echo 1 || echo 0)

ifeq ($(GIT_PRESENT),1)
GIT_TAGS:=$(shell cd $(PROJ_SRC_ROOT); git describe --tags 2> /dev/null 1>&2 && echo 1 || echo 0)
else
GIT_TAGS:=0
endif

.PHONY: $(CompileTimeInfoFile)
$(CompileTimeInfoFile):
	$(Verb) echo "Regenerating $(CompileTimeInfoFile)"
	$(Verb) echo '#define KLEE_BUILD_MODE "$(BuildMode)"' > $(CompileTimeInfoFile)
ifeq ($(GIT_PRESENT),1)
	$(Verb) echo '#define KLEE_BUILD_REVISION "'$(shell cd $(PROJ_SRC_ROOT); git rev-parse HEAD)'"' >> \
	$(CompileTimeInfoFile)
endif
ifeq ($(GIT_TAGS),1)
	$(Verb) echo '#define KLEE_BUILD_TAG "'$(shell cd $(PROJ_SRC_ROOT); git describe --tags)'"' >> \
	$(CompileTimeInfoFile)
endif
