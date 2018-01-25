# $Id$Revision$
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
        -I$(top_srcdir)/lib/ortho \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt \
	-I$(top_srcdir)/lib/pathplan

if WITH_CGRAPH
noinst_HEADERS = dot2.h dot2procs.h groups.h minc.h
noinst_LTLIBRARIES = libdotgen2_C.la
endif

libdotgen2_C_la_LDFLAGS = -no-undefined
libdotgen2_C_la_SOURCES = dotinit.c level.c minc2.c ns.c groups.c minc_utils.c decomp.c

# EXTRA_DIST = dotgen2.vcproj
