# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

AM_CPPFLAGS = \
        -I$(top_srcdir) \
        -I$(top_srcdir)/lib/common \
        -I$(top_srcdir)/lib/gvc \
        -I$(top_srcdir)/lib/neatogen \
        -I$(top_srcdir)/lib/pack \
        -I$(top_srcdir)/lib/pathplan \
        -I$(top_srcdir)/lib/sparse \
        -I$(top_srcdir)/lib/$(GRAPH) \
        -I$(top_srcdir)/lib/cdt

noinst_HEADERS = comp.h dbg.h fdp.h grid.h tlayout.h xlayout.h \
	clusteredges.h
noinst_LTLIBRARIES = libfdpgen_C.la

libfdpgen_C_la_SOURCES = comp.c dbg.c grid.c fdpinit.c layout.c \
	tlayout.c xlayout.c clusteredges.c

EXTRA_DIST = Makefile.old gvfdpgen.vcproj
