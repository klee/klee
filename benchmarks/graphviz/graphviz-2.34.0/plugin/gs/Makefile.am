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
	$(GS_CFLAGS) $(PANGOCAIRO_CFLAGS)

if WITH_GS
if WITH_PANGOCAIRO
noinst_LTLIBRARIES = libgvplugin_gs_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_gs.la
else
pkglib_LTLIBRARIES = libgvplugin_gs.la
endif
endif
endif

libgvplugin_gs_C_la_SOURCES = \
	gvplugin_gs.c \
	gvloadimage_gs.c

libgvplugin_gs_la_LDFLAGS = -version-info $(GVPLUGIN_VERSION_INFO)
libgvplugin_gs_la_SOURCES = $(libgvplugin_gs_C_la_SOURCES)
libgvplugin_gs_la_LIBADD = \
	$(top_builddir)/lib/gvc/libgvc.la $(PANGOCAIRO_LIBS) $(GS_LIBS)

if WITH_WIN32
libgvplugin_gs_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_gs_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_gs_LTX_library
endif

EXTRA_DIST = Makefile.old
