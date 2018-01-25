# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH 
GRAPH = cgraph 
else 
GRAPH = graph 
endif 

AM_CPPFLAGS = \
        -I$(top_srcdir) \
        -I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt 

noinst_HEADERS = ingraphs.h 
noinst_LTLIBRARIES = libingraphs_C.la

libingraphs_C_la_SOURCES = ingraphs.c

EXTRA_DIST = Makefile.old ingraphs.vcproj
