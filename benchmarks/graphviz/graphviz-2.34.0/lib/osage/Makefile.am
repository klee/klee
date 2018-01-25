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
	-I$(top_srcdir)/lib/neatogen \
	-I$(top_srcdir)/lib/fdpgen \
	-I$(top_srcdir)/lib/pack \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/sparse \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt

noinst_HEADERS = osage.h
noinst_LTLIBRARIES = libosage_C.la

libosage_C_la_SOURCES = osageinit.c
EXTRA_DIST = osage.vcproj


