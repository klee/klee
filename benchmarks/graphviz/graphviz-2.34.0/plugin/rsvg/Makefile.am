# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt \
	$(RSVG_CFLAGS) $(PANGOCAIRO_CFLAGS)

if WITH_RSVG
noinst_LTLIBRARIES = libgvplugin_rsvg_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_rsvg.la
else
pkglib_LTLIBRARIES = libgvplugin_rsvg.la
endif
endif

libgvplugin_rsvg_C_la_SOURCES = \
	gvplugin_rsvg.c \
	gvloadimage_rsvg.c

libgvplugin_rsvg_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_rsvg_la_SOURCES = $(libgvplugin_rsvg_C_la_SOURCES)
libgvplugin_rsvg_la_LIBADD = \
	$(top_builddir)/lib/gvc/libgvc.la @RSVG_LIBS@ @PANGOCAIRO_LIBS@

if WITH_WIN32
libgvplugin_rsvg_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_rsvg_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_rsvg_LTX_library
endif

EXTRA_DIST = Makefile.old

