# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

pdfdir = $(pkgdatadir)/doc/pdf

AM_CPPFLAGS = \
        -I$(top_srcdir) \
        -I$(top_srcdir)/lib/ingraphs \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

bin_PROGRAMS = prune
noinst_HEADERS = generic_list.h
man_MANS = prune.1
pdf_DATA = prune.pdf

prune_SOURCES = generic_list.c prune.c

prune_LDADD = $(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(top_builddir)/lib/cdt/libcdt.la

.1.pdf:
	- @GROFF@ -Tps -man $< | @PS2PDF@ - - >$@

EXTRA_DIST = $(man_MANS) $(pdf_DATA) Makefile.old README test.gv prune.vcproj


DISTCLEANFILES = $(pdf_DATA)
