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
	$(LIBGVC_CFLAGS)

noinst_LTLIBRARIES = libgvplugin_core_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_core.la
else
pkglib_LTLIBRARIES = libgvplugin_core.la
endif

libgvplugin_core_C_la_SOURCES = \
	gvplugin_core.c \
	gvrender_core_dot.c \
	gvrender_core_fig.c \
	gvrender_core_map.c \
	gvrender_core_ps.c \
	gvrender_core_svg.c \
	gvrender_core_tk.c \
	gvrender_core_vml.c \
	gvrender_core_pov.c \
	gvrender_core_pic.c \
	gvloadimage_core.c \
	ps.txt

libgvplugin_core_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_core_la_SOURCES = $(libgvplugin_core_C_la_SOURCES)
libgvplugin_core_la_LIBADD = $(top_builddir)/lib/gvc/libgvc.la \
				$(top_builddir)/lib/$(GRAPH)/lib$(GRAPH).la \
				$(top_builddir)/lib/cdt/libcdt.la

gvrender_core_ps.o gvrender_core_ps.lo: ps.h

ps.h : $(srcdir)/ps.txt
	$(AWK) -f $(top_srcdir)/awk/stringize.awk $(srcdir)/ps.txt > ps.h

if WITH_WIN32
libgvplugin_core_la_LDFLAGS += -no-undefined
libgvplugin_core_la_LIBADD += $(top_builddir)/lib/$(GRAPH)/lib$(GRAPH).la
endif

if WITH_DARWIN9
libgvplugin_core_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_core_LTX_library
endif

EXTRA_DIST = ps.h Makefile.old plugincore.vcproj gvplugin_core.vcproj gvplugin_core.def

DISTCLEANFILES = ps.h

