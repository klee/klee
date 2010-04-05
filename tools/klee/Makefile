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
STP_LIBS := stp_c_interface.a stp_AST.a stp_bitvec.a stp_constantbv.a stp_sat.a \
	stp_simplifier.a
USEDLIBS = kleeCore.a kleeModule.a kleaverSolver.a kleaverExpr.a kleeSupport.a kleeBasic.a \
	$(STP_LIBS)
LINK_COMPONENTS = jit bitreader bitwriter ipo linker engine

include $(LEVEL)/Makefile.common
