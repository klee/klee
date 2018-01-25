# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

SUBDIRS = lib

pdfdir = $(pkgdatadir)/doc/pdf

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/expr \
	-I$(top_builddir)/lib/expr \
	-I$(top_srcdir)/lib/gvpr \
	-I$(top_srcdir)/lib/vmalloc \
	-I$(top_srcdir)/lib/sfio \
	-I$(top_srcdir)/lib/ast \
	-I$(top_srcdir)/lib/ingraphs \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

if ENABLE_STATIC
bin_PROGRAMS = gvpr gvpr_static
else
bin_PROGRAMS = gvpr
endif

man_MANS = gvpr.1
pdf_DATA = gvpr.1.pdf

gvpr_SOURCES = gvprmain.c
gvpr_CPPFLAGS = $(AM_CPPFLAGS)
gvpr_LDADD = $(top_builddir)/lib/gvpr/libgvpr.la

gvpr.1.pdf: $(srcdir)/gvpr.1
	- @GROFF@ -Tps -man -t $(srcdir)/gvpr.1 | @PS2PDF@ - - >gvpr.1.pdf

gvpr_static_SOURCES = gvprmain.c
gvpr_static_CPPFLAGS = $(AM_CPPFLAGS)
gvpr_static_LDADD = \
	$(top_builddir)/lib/gvpr/libgvpr_C.la \
	$(top_builddir)/lib/expr/libexpr_C.la \
	$(top_builddir)/lib/sfio/libsfio_C.la \
	$(top_builddir)/lib/vmalloc/libvmalloc_C.la \
	$(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/ast/libast_C.la \
	$(top_builddir)/lib/cgraph/libcgraph_C.la \
	$(top_builddir)/lib/cdt/libcdt_C.la \
       	$(MATH_LIBS)

EXTRA_DIST = $(man_MANS) $(pdf_DATA) lib Makefile.old gvpr.vcproj

DISTCLEANFILES = $(pdf_DATA)
