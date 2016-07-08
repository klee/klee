#===-- tools/kleaver/Makefile ------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..
TOOLNAME = kleaver

include $(LEVEL)/Makefile.config

# FIXME: Ideally we wouldn't have any LLVM dependencies here, which
# means kicking out klee's Support.
USEDLIBS = kleeBasic.a kleaverSolver.a kleaverExpr.a kleeSupport.a 
LINK_COMPONENTS = support

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
