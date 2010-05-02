#===-- stp/bitvec/Makefile ---------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..

LIBRARYNAME=stp_bitvec
DONT_BUILD_RELINKED=1
BUILD_ARCHIVE=1

include $(LEVEL)/Makefile.common

# HACK: Force -Wno-deprecated for ext container use.
CXX.Flags += -Wno-deprecated
