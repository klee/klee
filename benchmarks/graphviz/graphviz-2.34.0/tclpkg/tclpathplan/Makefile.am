# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

SUBDIRS = demo

pkgtcldir = $(pkglibdir)/tcl

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/tclpkg/tclhandle \
	-I$(top_srcdir)/lib/pathplan @TCL_INCLUDES@

AM_CFLAGS = @TCL_CFLAGS@

noinst_HEADERS = simple.h
if WITH_TCL
pkgtcl_LTLIBRARIES = libtclplan.la
endif

libtclplan_la_SOURCES = find_ints.c intersect.c makecw.c tclpathplan.c wrapper.c
libtclplan_la_LDFLAGS = -no-undefined
libtclplan_la_LIBADD = \
	$(top_builddir)/tclpkg/tclhandle/libtclhandle_C.la \
	$(top_builddir)/tclpkg/tclstubs/libtclstubs_C.la \
	$(top_builddir)/lib/pathplan/libpathplan.la $(MATH_LIBS)

if WITH_TCL
all-local: pkgIndex.tcl

pkgIndex.tcl: libtclplan.la
	sh $(top_srcdir)/tclpkg/mkpkgindex.sh libtclplan.la Tclpathplan $(VERSION)
endif

CLEANFILES = pkgIndex.tcl

EXTRA_DIST = $(libtclplan_la_SOURCES)
