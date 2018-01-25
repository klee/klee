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
	$(PANGOCAIRO_CFLAGS) $(GDK_PIXBUF_CFLAGS)

if WITH_PANGOCAIRO
if WITH_GDK_PIXBUF
noinst_LTLIBRARIES = libgvplugin_gdk_pixbuf_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_gdk_pixbuf.la
else
pkglib_LTLIBRARIES = libgvplugin_gdk_pixbuf.la
endif
endif
endif

libgvplugin_gdk_pixbuf_C_la_SOURCES = \
	gvplugin_gdk_pixbuf.c \
	gvdevice_gdk_pixbuf.c \
	gvloadimage_gdk_pixbuf.c

libgvplugin_gdk_pixbuf_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_gdk_pixbuf_la_SOURCES = $(libgvplugin_gdk_pixbuf_C_la_SOURCES)
libgvplugin_gdk_pixbuf_la_LIBADD = \
	$(top_builddir)/lib/gvc/libgvc.la @GDK_PIXBUF_LIBS@
	
if WITH_WIN32
libgvplugin_gdk_pixbuf_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_gdk_pixbuf_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_gdk_pixbuf_LTX_library
endif

EXTRA_DIST = Makefile.old

