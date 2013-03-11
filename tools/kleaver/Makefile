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

LIBS += -lstp
