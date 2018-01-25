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
	-I$(top_srcdir)/lib/fdpgen \
	-I$(top_srcdir)/lib/pack \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/sparse \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt

noinst_HEADERS = patchwork.h tree_map.h
noinst_LTLIBRARIES = libpatchwork_C.la

libpatchwork_C_la_SOURCES = patchwork.c patchworkinit.c tree_map.c

EXTRA_DIST = Makefile.old gvpatchwork.vcproj
