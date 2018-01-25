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
	$(QUARTZ_CFLAGS)

noinst_HEADERS = gvplugin_quartz.h GVTextLayout.h

if WITH_QUARTZ
noinst_LTLIBRARIES = libgvplugin_quartz_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_quartz.la
else
pkglib_LTLIBRARIES = libgvplugin_quartz.la
endif
endif

libgvplugin_quartz_C_la_SOURCES = \
	gvdevice_quartz.c \
	gvloadimage_quartz.c \
	gvplugin_quartz.c \
	gvrender_quartz.c \
	gvtextlayout_quartz.c \
	GVTextLayout.m

libgvplugin_quartz_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@ -framework CoreFoundation
libgvplugin_quartz_la_SOURCES = $(libgvplugin_quartz_C_la_SOURCES)
libgvplugin_quartz_la_LIBADD = @QUARTZ_LIBS@

if WITH_WIN32
libgvplugin_quartz_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_quartz_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_quartz_LTX_library
endif
