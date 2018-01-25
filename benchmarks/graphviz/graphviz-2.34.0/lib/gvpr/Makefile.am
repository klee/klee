# $Id$Revision: 
## Process this file with automake to produce Makefile.in

pkgconfigdir = $(libdir)/pkgconfig

GVPR_VERSION="2:0:0"

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/expr \
	-I$(top_builddir)/lib/expr \
	-I$(top_srcdir)/lib/vmalloc \
	-I$(top_srcdir)/lib/sfio \
	-I$(top_srcdir)/lib/ast \
	-I$(top_srcdir)/lib/ingraphs \
	-I$(top_srcdir)/lib/common \
	-I$(top_builddir)/lib/common \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt \
	-DDFLT_GVPRPATH="\".$(PATH_SEPARATOR)$(pkgdatadir)/gvpr\""

pkginclude_HEADERS = gvpr.h
noinst_PROGRAMS = mkdefs
noinst_HEADERS = actions.h compile.h gprstate.h parse.h queue.h trieFA.h trie.c
noinst_LTLIBRARIES = libgvpr_C.la
lib_LTLIBRARIES = libgvpr.la
pkgconfig_DATA = libgvpr.pc

libgvpr_C_la_SOURCES = actions.c compile.c gvpr.c gprstate.c parse.c queue.c 
$(libgvpr_C_la_OBJECTS): gdefs.h
libgvpr_C_la_DEPENDENCIES = \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/common/libcommon_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la


libgvpr_la_LDFLAGS = -version-info $(GVPR_VERSION) -no-undefined
libgvpr_la_SOURCES = $(libgvpr_C_la_SOURCES)
libgvpr_la_LIBADD = $(libgvpr_C_la_LIBADD) \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/cdt/libcdt.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(MATH_LIBS)
libgvpr_la_DEPENDENCIES = $(libgvpr_C_la_DEPENDENCIES)

mkdefs_SOURCES = mkdefs.c

gdefs.h: $(top_srcdir)/lib/gvpr/gprdata mkdefs
	./mkdefs gdefs.h < $(top_srcdir)/lib/gvpr/gprdata

EXTRA_DIST = $(man_MANS) $(pdf_DATA) Makefile.old gprdata mkdefs.c gdefs.h trie.c gvprlib.vcproj

DISTCLEANFILES = $(pdf_DATA) gdefs.h mkdefs
