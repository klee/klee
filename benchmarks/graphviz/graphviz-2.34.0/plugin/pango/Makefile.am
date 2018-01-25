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
	-I$(top_builddir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt \
	$(PANGOCAIRO_CFLAGS) $(FREETYPE2_CFLAGS) $(LIBGVC_CFLAGS)

noinst_HEADERS = gvplugin_pango.h gvgetfontlist.h

if WITH_PANGOCAIRO
noinst_LTLIBRARIES = libgvplugin_pango_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_pango.la
else
pkglib_LTLIBRARIES = libgvplugin_pango.la
endif
endif

libgvplugin_pango_C_la_SOURCES = \
	gvplugin_pango.c \
	gvrender_pango.c \
	gvgetfontlist_pango.c \
	gvtextlayout_pango.c \
	gvloadimage_pango.c

libgvplugin_pango_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_pango_la_SOURCES = $(libgvplugin_pango_C_la_SOURCES)
libgvplugin_pango_la_LIBADD = \
	$(top_builddir)/lib/gvc/libgvc.la @PANGOCAIRO_LIBS@ @PANGOFT2_LIBS@ @MATH_LIBS@

if WITH_WIN32
libgvplugin_pango_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_pango_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_pango_LTX_library -framework CoreFoundation -framework CoreServices
endif

EXTRA_DIST = Makefile.old gvplugin_pango.vcproj

