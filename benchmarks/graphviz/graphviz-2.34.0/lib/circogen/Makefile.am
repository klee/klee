# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

pkgconfigdir = $(libdir)/pkgconfig

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

noinst_HEADERS = block.h blockpath.h blocktree.h circo.h \
	circpos.h circular.h deglist.h edgelist.h nodelist.h \
	nodeset.h 
noinst_LTLIBRARIES = libcircogen_C.la

libcircogen_C_la_SOURCES = circularinit.c nodelist.c block.c edgelist.c \
	circular.c deglist.c blocktree.c blockpath.c \
	circpos.c nodeset.c

EXTRA_DIST = Makefile.old gvcircogen.vcproj
