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
	$(GTK_CFLAGS)

if WITH_PANGOCAIRO
if WITH_GTK
noinst_HEADERS = support.h interface.h callbacks.h
noinst_LTLIBRARIES = libgvplugin_gtk_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_gtk.la
else
pkglib_LTLIBRARIES = libgvplugin_gtk.la
endif
endif
endif

libgvplugin_gtk_C_la_SOURCES = \
	gvplugin_gtk.c \
	gvdevice_gtk.c \
	support.c \
        interface.c \
        callbacks.c 

libgvplugin_gtk_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_gtk_la_SOURCES = $(libgvplugin_gtk_C_la_SOURCES)
libgvplugin_gtk_la_LIBADD = $(GTK_LIBS)

if WITH_WIN32
libgvplugin_gtk_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_gtk_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_gtk_LTX_library
endif

EXTRA_DIST = gtk.glade gtk.gladep 
