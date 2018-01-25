# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

pkgfontdir = ${pkgdatadir}/font

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt \
	$(MING_CFLAGS)

if WITH_MING
noinst_LTLIBRARIES = libgvplugin_ming_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_ming.la
else
pkglib_LTLIBRARIES = libgvplugin_ming.la
endif
pkgfont_DATA = Bitstream_Vera_Sans.fdb  Bitstream_Vera_Serif.fdb
endif

libgvplugin_ming_C_la_SOURCES = \
	gvplugin_ming.c \
	gvrender_ming.c

libgvplugin_ming_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_ming_la_SOURCES = $(libgvplugin_ming_C_la_SOURCES)
libgvplugin_ming_la_LIBADD = @MING_LIBS@ @MATH_LIBS@

if WITH_WIN32
libgvplugin_ming_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_ming_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_ming_LTX_library
endif

EXTRA_DIST = Makefile.old Bitstream_Vera_Sans.fdb Bitstream_Vera_Serif.fdb
