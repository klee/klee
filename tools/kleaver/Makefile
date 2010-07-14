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

STP_LIBS := stp_c_interface.a stp_AST.a stp_bitvec.a stp_constantbv.a stp_sat.a \
	stp_simplifier.a
# FIXME: Ideally we wouldn't have any LLVM dependencies here, which
# means kicking out klee's Support.
USEDLIBS = kleaverSolver.a kleaverExpr.a kleeSupport.a kleeBasic.a
ifeq ($(ENABLE_EXT_STP),0)
  USEDLIBS += $(STP_LIBS)
endif
LINK_COMPONENTS = support

include $(LEVEL)/Makefile.common

ifeq ($(ENABLE_EXT_STP),1)
  LIBS += -lstp
endif
