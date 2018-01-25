# $Id$Revision: 
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/sparse \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/sfdpgen \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt $(ANN_CFLAGS)

noinst_HEADERS =  edge_bundling.h ink.h agglomerative_bundling.h nearest_neighbor_graph.h nearest_neighbor_graph_ann.h

if WITH_ANN
noinst_LTLIBRARIES = libmingle_C.la
endif

libmingle_C_la_SOURCES = edge_bundling.c ink.c agglomerative_bundling.c nearest_neighbor_graph.c nearest_neighbor_graph_ann.cpp

EXTRA_DIST = minglelib.vcproj
 
