#===-- runtime/Intrinsic/Makefile --------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..

# FIXME: This doesn't work. So we have to duplicate
# functions.c via a symlink
#SOURCES=../svcomp64/functions.c

# Needed for LLVM version
include $(LEVEL)/Makefile.config

ifeq ($(shell python -c "print($(LLVM_VERSION_MAJOR).$(LLVM_VERSION_MINOR) >= 3.3)"), True)
# For these versions of LLVM KLEE expects kleeRuntimeIntrinsic to be a LLVM module rather than an archive
MODULE_NAME=kleeSVCOMPRuntime32
else
# KLEE built against older versions of LLVM expects a library archive instead
BYTECODE_LIBRARY=1
LIBRARYNAME=kleeSVCOMPRuntime32
endif

DONT_BUILD_RELINKED=1
# Don't strip debug info from the module.
DEBUG_RUNTIME=1
NO_PEDANTIC=1
NO_BUILD_ARCHIVE=1

C.Flags += -fno-builtin -m32

include $(LEVEL)/Makefile.common
