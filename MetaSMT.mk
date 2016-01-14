# This file contains common code for linking against MetaSMT
#
# FIXME: This is a horribly fragile hack.
# Instead the detection of what solvers the build of MetaSMT supports should be
# done by the configure script and then the appropriate compiler flags should
# be emitted to Makefile.config for consumption here.
ifeq ($(ENABLE_METASMT),1)
  include $(METASMT_ROOT)/share/metaSMT/metaSMT.makefile
  LD.Flags += -L$(METASMT_ROOT)/../../deps/Z3-4.1/lib \
              -L$(METASMT_ROOT)/../../deps/boolector-1.5.118/lib \
              -L$(METASMT_ROOT)/../../deps/minisat-git/lib/ \
              -L$(METASMT_ROOT)/../../deps/boost-1_52_0/lib \
              -L$(METASMT_ROOT)/../../deps/stp-svn/lib
  CXX.Flags += -DBOOST_HAS_GCC_TR1
  CXX.Flags := $(filter-out -fno-exceptions,$(CXX.Flags))
  LIBS += -lgomp -lboost_iostreams -lboost_thread -lboost_system -lmetaSMT -lz3 -lstp -lrt -lboolector -lminisat_core
endif
