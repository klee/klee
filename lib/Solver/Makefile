#===-- lib/Solver/Makefile ---------------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..

LIBRARYNAME=kleaverSolver
DONT_BUILD_RELINKED=1
BUILD_ARCHIVE=1
NO_INSTALL=1

include $(LEVEL)/Makefile.common

ifeq ($(ENABLE_METASMT),1)
  include $(METASMT_ROOT)/share/metaSMT/metaSMT.makefile
  CXX.Flags += -DBOOST_HAS_GCC_TR1
  CXX.Flags := $(filter-out -fno-exceptions,$(CXX.Flags))
  CXX.Flags := $(filter-out -fno-rtti,$(CXX.Flags))
  CXX.Flags += $(metaSMT_CXXFLAGS)
  CXX.Flags += $(metaSMT_INCLUDES)
endif
