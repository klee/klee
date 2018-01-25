# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

pdfdir = $(pkgdatadir)/doc/pdf

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt \
	$(PANGOCAIRO_CFLAGS)

if WITH_PANGOCAIRO
if WITH_X
noinst_LTLIBRARIES = libgvplugin_xlib_C.la
if WITH_WIN32
lib_LTLIBRARIES = libgvplugin_xlib.la
else
pkglib_LTLIBRARIES = libgvplugin_xlib.la
endif
bin_SCRIPTS = vimdot
endif
endif

man_MANS = vimdot.1
pdf_DATA = vimdot.1.pdf

vimdot.1.pdf: $(srcdir)/vimdot.1
	- $(GROFF) -Tps -man $(srcdir)/vimdot.1 | $(PS2PDF) - - >vimdot.1.pdf

libgvplugin_xlib_C_la_SOURCES = \
	gvplugin_xlib.c \
	gvdevice_xlib.c

libgvplugin_xlib_la_LDFLAGS = -version-info @GVPLUGIN_VERSION_INFO@
libgvplugin_xlib_la_SOURCES = $(libgvplugin_xlib_C_la_SOURCES)
libgvplugin_xlib_la_LIBADD = @PANGOCAIRO_LIBS@ $(X11_LIBS) $(XRENDER_LIBS) @SOCKET_LIBS@ @MATH_LIBS@

if WITH_WIN32
libgvplugin_xlib_la_LDFLAGS += -no-undefined
endif

if WITH_DARWIN9
libgvplugin_xlib_la_LDFLAGS += -Wl,-exported_symbol,_gvplugin_xlib_LTX_library
endif

vimdot: $(top_srcdir)/plugin/xlib/vimdot.sh
	cp $(top_srcdir)/plugin/xlib/vimdot.sh vimdot
	chmod +x vimdot

EXTRA_DIST = vimdot.sh vimdot.1 vimdot.1.pdf

DISTCLEANFILES = vimdot vimdot.1.pdf
