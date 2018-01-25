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
	$(PANGOCAIRO_CFLAGS) $(GD_INCLUDES) $(LIBGVC_CFLAGS)

if WITH_LIBGD
noinst_LTLIBRARIES = libgvplugin_gd_C.la

if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_gd.la
else
pkglib_LTLIBRARIES = libgvplugin_gd.la
endif
endif

libgvplugin_gd_C_la_SOURCES = \
	gvplugin_gd.c \
	gvrender_gd.c \
	gvrender_gd_vrml.c \
	gvtextlayout_gd.c \
	gvloadimage_gd.c \
	gvdevice_gd.c

libgvplugin_gd_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_gd_la_SOURCES = $(libgvplugin_gd_C_la_SOURCES)

if WITH_LIBGD
libgvplugin_gd_la_LIBADD = \
	$(top_builddir)/lib/gvc/libgvc.la \
	$(top_builddir)/lib/$(GRAPH)/lib$(GRAPH).la \
	$(top_builddir)/lib/cdt/libcdt.la @PANGOCAIRO_LIBS@ @GD_LIBS@ @MATH_LIBS@
libgvplugin_gd_la_DEPENDENCIES =  $(top_builddir)/lib/gvc/libgvc.la

if WITH_WIN32
libgvplugin_gd_la_LDFLAGS += -no-undefined
libgvplugin_gd_la_LIBADD += $(top_builddir)/lib/$(GRAPH)/lib$(GRAPH).la $(top_builddir)/lib/pathplan/libpathplan.la
endif

if WITH_DARWIN9
libgvplugin_gd_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_gd_LTX_library
endif
endif

EXTRA_DIST = Makefile.old gvplugin_gd.vcproj gvplugin_gd.def

