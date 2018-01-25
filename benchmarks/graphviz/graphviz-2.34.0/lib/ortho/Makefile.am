# $Id$Revision: 
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
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt

if WITH_ORTHO
AM_CFLAGS = -D_BLD_gvc=1
endif

noinst_HEADERS = fPQ.h ortho.h maze.h partition.h rawgraph.h sgraph.h structures.h trap.h

if WITH_ORTHO
noinst_LTLIBRARIES = libortho_C.la
endif

libortho_C_la_SOURCES = fPQ.c maze.c ortho.c partition.c rawgraph.c sgraph.c trapezoid.c

EXTRA_DIST = Makefile.old gvortho.vcproj
 
