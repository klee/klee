# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

SUBDIRS = demo

pkgtcldir = $(pkglibdir)/tcl
pdfdir = $(pkgdatadir)/doc/pdf

man = tkspline.3tk
pdf = tkspline.3tk.pdf

# If all else fails there is a local copy of tkInt.h in tkstubs/
AM_CPPFLAGS = \
	-I@TK_SRC_DIR@/generic \
	-I@TCL_SRC_DIR@/generic \
	@TCL_INCLUDES@ @TCLINT_INCLUDES@ \
	@TK_INCLUDES@ @TKINT_INCLUDES@ -I$(top_srcdir)/tclpkg/tkstubs

AM_CFLAGS = @X_CFLAGS@ @TCL_CFLAGS@  @TK_CFLAGS@ @TK_DEFS@ \
	-UPACKAGE_NAME -UPACKAGE_STRING -UPACKAGE_TARNAME \
	-UPACKAGE_VERSION -UPACKAGE_BUGREPORT

if WITH_TK
pkgtcl_LTLIBRARIES = libtkspline.la
man_MANS = $(man)
pdf_DATA = $(pdf)
endif

libtkspline_la_LDFLAGS = -no-undefined 
libtkspline_la_SOURCES = tkspline.c
libtkspline_la_LIBADD = \
	$(top_builddir)/tclpkg/tkstubs/libtkstubs_C.la \
	$(top_builddir)/tclpkg/tclstubs/libtclstubs_C.la @TK_LIB_SPEC@

if WITH_TK
all-local: pkgIndex.tcl

pkgIndex.tcl: libtkspline.la
	sh $(top_srcdir)/tclpkg/mkpkgindex.sh libtkspline.la Tkspline $(VERSION)
endif

tkspline.3tk.pdf: $(srcdir)/tkspline.3tk
	- @GROFF@ -Tps -man $(srcdir)/tkspline.3tk | @PS2PDF@ - - >tkspline.3tk.pdf

EXTRA_DIST = $(man) $(pdf) dllEntry.c

CLEANFILES = pkgIndex.tcl

DISTCLEANFILES = $(pdf)
