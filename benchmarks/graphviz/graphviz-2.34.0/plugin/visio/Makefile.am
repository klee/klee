# $Id: Makefile.am,v 1.3 2009/06/03 01:10:57 ellson Exp $ $Revision: 1.3 $
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
	$(VISIO_CFLAGS) 

noinst_HEADERS = VisioGraphic.h VisioRender.h VisioText.h

if WITH_VISIO
noinst_LTLIBRARIES = libgvplugin_visio_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_visio.la
else
pkglib_LTLIBRARIES = libgvplugin_visio.la
endif
endif

libgvplugin_visio_C_la_SOURCES = \
	gvplugin_visio.c \
	VisioGraphic.cpp \
	VisioRender.cpp \
	VisioText.cpp \
	gvrender_visio_vdx.cpp

libgvplugin_visio_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_visio_la_SOURCES = $(libgvplugin_visio_C_la_SOURCES)
libgvplugin_visio_la_LIBADD = \
	$(top_builddir)/lib/gvc/libgvc.la @VISIO_LIBS@ 

if WITH_WIN32
libgvplugin_visio_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_visio_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_visio_LTX_library
endif

EXTRA_DIST = 

DISTCLEANFILES = 
