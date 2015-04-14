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

LIBS += $(STP_LDFLAGS)

ifeq ($(ENABLE_METASMT),1)
  include $(METASMT_ROOT)/share/metaSMT/metaSMT.makefile
  LD.Flags += -L$(METASMT_ROOT)/../../deps/Z3-4.1/lib \
              -L$(METASMT_ROOT)/../../deps/boolector-1.5.118/lib \
              -L$(METASMT_ROOT)/../../deps/minisat-git/lib/ \
              -L$(METASMT_ROOT)/../../deps/boost-1_52_0/lib 
  CXX.Flags += -DBOOST_HAS_GCC_TR1
  CXX.Flags := $(filter-out -fno-exceptions,$(CXX.Flags)) 
  LIBS += -lgomp -lboost_iostreams -lboost_thread -lboost_system -lmetaSMT -lz3 -lrt -lboolector -lminisat_core
endif
