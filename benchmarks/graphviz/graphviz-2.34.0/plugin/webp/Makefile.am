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
	$(PANGOCAIRO_CFLAGS) $(WEBP_CFLAGS)

if WITH_WEBP
noinst_LTLIBRARIES = libgvplugin_webp_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_webp.la
else
pkglib_LTLIBRARIES = libgvplugin_webp.la
endif
endif

libgvplugin_webp_C_la_SOURCES = \
	gvplugin_webp.c \
	gvloadimage_webp.c \
	gvdevice_webp.c

libgvplugin_webp_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_webp_la_SOURCES = $(libgvplugin_webp_C_la_SOURCES)
libgvplugin_webp_la_LIBADD = @WEBP_LIBS@ @PANGOCAIRO_LIBS@

if WITH_WIN32
libgvplugin_webp_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_webp_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_webp_LTX_library
endif
