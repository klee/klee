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

include $(LEVEL)/Makefile.common

LIBS += -lstp
