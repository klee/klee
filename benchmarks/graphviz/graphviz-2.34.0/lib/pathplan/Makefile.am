# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

PATHPLAN_VERSION="4:0:0"

pdfdir = $(pkgdatadir)/doc/pdf
pkgconfigdir = $(libdir)/pkgconfig

if WITH_WIN32
AM_CPPFLAGS = -O0
endif

pkginclude_HEADERS = pathgeom.h pathplan.h
noinst_HEADERS = pathutil.h solvers.h tri.h vis.h vispath.h

noinst_LTLIBRARIES = libpathplan_C.la
lib_LTLIBRARIES = libpathplan.la
pkgconfig_DATA = libpathplan.pc
man_MANS = pathplan.3
pdf_DATA = pathplan.3.pdf

libpathplan_C_la_SOURCES = cvt.c inpoly.c route.c shortest.c \
	shortestpth.c solvers.c triang.c util.c visibility.c

libpathplan_la_LDFLAGS = -version-info $(PATHPLAN_VERSION) -no-undefined
libpathplan_la_SOURCES = $(libpathplan_C_la_SOURCES)
libpathplan_la_LIBADD = @MATH_LIBS@

pathplan.3.pdf: $(srcdir)/pathplan.3
	- @GROFF@ -Tps -man $(srcdir)/pathplan.3 | @PS2PDF@ - - >pathplan.3.pdf

EXTRA_DIST = $(man_MANS) $(pdf_DATA) Makefile.old path.lefty Pathplan.vcproj pathplan.def

DISTCLEANFILES = $(pdf_DATA)
