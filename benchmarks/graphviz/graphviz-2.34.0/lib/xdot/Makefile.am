# $Id$ $Revision$
# ## Process this file with automake to produce Makefile.in

pkgconfigdir = $(libdir)/pkgconfig

XDOT_VERSION="4:0:0"

pdfdir = $(pkgdatadir)/doc/pdf
pkginclude_HEADERS = xdot.h

noinst_LTLIBRARIES = libxdot_C.la
lib_LTLIBRARIES = libxdot.la
pkgconfig_DATA = libxdot.pc

man_MANS = xdot.3
pdf_DATA = xdot.3.pdf

libxdot_C_la_SOURCES = xdot.c
libxdot_la_LDFLAGS = -version-info $(XDOT_VERSION) -no-undefined
libxdot_la_SOURCES = $(libxdot_C_la_SOURCES)

xdot.3.pdf: $(srcdir)/xdot.3
	- @GROFF@ -Tps -man $(srcdir)/xdot.3 | @PS2PDF@ - - >xdot.3.pdf

EXTRA_DIST = $(man_MANS) $(pdf_DATA) xdot.vcproj

DISTCLEANFILES = $(pdf_DATA)

