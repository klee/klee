# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

pdfdir = $(pkgdatadir)/doc/pdf

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/sparse \
	-I$(top_srcdir)/lib/sfdpgen \
	-I$(top_srcdir)/lib/mingle \
	-I$(top_srcdir)/lib/ingraphs \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt

if WITH_ANN
bin_PROGRAMS = mingle

man_MANS = mingle.1
pdf_DATA = mingle.1.pdf
endif

mingle_SOURCES = minglemain.c
mingle_CPPFLAGS = $(AM_CPPFLAGS)
mingle_LDADD = \
    $(top_builddir)/lib/mingle/libmingle_C.la \
    $(top_builddir)/lib/ingraphs/libingraphs_C.la \
	$(top_builddir)/lib/sfdpgen/libsfdpgen_C.la \
	$(top_builddir)/lib/neatogen/libneatogen_C.la \
	$(top_builddir)/lib/sparse/libsparse_C.la \
	$(top_builddir)/lib/rbtree/librbtree_C.la \
	$(top_builddir)/lib/common/libcommon_C.la \
	$(top_builddir)/lib/cgraph/libcgraph.la \
	$(ANN_LIBS) -lstdc++ -lm

mingle.1.pdf: $(srcdir)/mingle.1
	- @GROFF@ -e -Tps -man -t $(srcdir)/mingle.1 | @PS2PDF@ - - >mingle.1.pdf

EXTRA_DIST = $(man_MANS) $(pdf_DATA) mingle.vcproj

DISTCLEANFILES = $(pdf_DATA)
