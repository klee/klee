#===-- tools/klee/Makefile ---------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..
TOOLNAME = klee

include $(LEVEL)/Makefile.config

USEDLIBS = kleeCore.a kleeBasic.a kleeModule.a  kleaverSolver.a kleaverExpr.a kleeSupport.a 
LINK_COMPONENTS = jit bitreader bitwriter ipo linker engine

ifeq ($(shell python -c "print($(LLVM_VERSION_MAJOR).$(LLVM_VERSION_MINOR) >= 3.3)"), True)
LINK_COMPONENTS += irreader
endif
include $(LEVEL)/Makefile.common

ifneq ($(ENABLE_STP),0)
  LIBS += $(STP_LDFLAGS)
endif

ifneq ($(ENABLE_Z3),0)
  LIBS += $(Z3_LDFLAGS)
endif

include $(PROJ_SRC_ROOT)/MetaSMT.mk

ifeq ($(HAVE_TCMALLOC),1)
  LIBS += $(TCMALLOC_LIB)
endif

ifeq ($(HAVE_ZLIB),1)
  LIBS += -lz
endif