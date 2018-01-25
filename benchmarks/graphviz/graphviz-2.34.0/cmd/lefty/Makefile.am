# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

SUBDIRS = ws os dot2l examples

# lefty build parameters
#not useful / old:
#  FEATURE_CS          coshell support
#  FEATURE_NETSCAPE    build lefty as a netscape plugin
#  FEATURE_RUSAGE      print rusage stats on exit
#  FEATURE_VFORK       use vfork instead of fork
#  FEATURE_GMAP        build performer version of lefty (source in lefty3d)
#  FEATURE_MESAGL      assume MesaGL instead of SGI's OpenGL for GMAP
# FEATURE_MINTSIZE     use int instead of short to hold object sizes.
#                      allows for larger objects (strings, hash tables, etc.)
# FEATURE_MS           assume windows system calls (as opposed to UNIX)
#                      to use when building on ms windows without UWIN
# FEATURE_X11          assume X11 graphics calls
# FEATURE_BACKINGSTORE ask X server to use backing store for canvas
# FEATURE_NEXTAW       when linking with -lnextaw a widget lib ala NeXT
# FEATURE_XAW3D        when linking with -lXaw3d a nicer looking widget lib
# FEATURE_WIN32        assume WIN32 graphics calls
# FEATURE_DOT          when linking in the dot2l code, a graph parser

leftydir = $(pkgdatadir)/lefty
pdfdir = $(pkgdatadir)/doc/pdf

if WITH_WIN32
AM_CPPFLAGS = \
	-I$(top_srcdir)/cmd/lefty/ws/mswin32 \
	-I$(top_srcdir)/cmd/lefty/os/mswin32 \
	-I$(top_srcdir)/cmd/lefty/dot2l @XAW_INCLUDES@

AM_CFLAGS = -DHAVE_CONFIG_H @X_CFLAGS@
else
AM_CPPFLAGS = \
	-I$(top_srcdir)/cmd/lefty/ws/x11 \
	-I$(top_srcdir)/cmd/lefty/os/unix \
	-I$(top_srcdir)/cmd/lefty/dot2l @XAW_INCLUDES@

AM_CFLAGS = -DLEFTYDATADIR=\"$(leftydir)\" -DHAVE_CONFIG_H @X_CFLAGS@
endif


man = lefty.1
pdf = lefty.1.pdf

noinst_HEADERS = code.h common.h display.h exec.h g.h gfxview.h internal.h \
	io.h lex.h mem.h parse.h str.h tbl.h txtview.h

if WITH_X
if WITH_XAW
bin_PROGRAMS = lefty
man_MANS = $(man)
pdf_DATA = $(pdf)
endif
endif
if WITH_WIN32
bin_PROGRAMS = lefty
man_MANS = $(man)
pdf_DATA = $(pdf)
endif

lefty_DATA = lefty.psp

lefty_SOURCES = code.c common.c display.c exec.c g.c gfxview.c internal.c \
	lefty.c lex.c mem.c parse.c str.c tbl.c txtview.c

#
# NB. -lXaw must be before -lXt or strange inability-to-get-mouse-focus
# problems can occur on some systems.
#
if WITH_WIN32
lefty_LDADD = lefty.res $(top_builddir)/cmd/lefty/dot2l/libdot2l.la \
	$(top_builddir)/cmd/lefty/ws/mswin32/libws.la \
	$(top_builddir)/cmd/lefty/os/mswin32/libos.la \
	-luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 @MATH_LIBS@

if RC_IS_RC
lefty.res : ws/mswin32/lefty.rc
	rc -l 0x409 -Iws/mswin32 '-IC:\\progra~1\\microsoft visual studio 8\\vc\\PlatformSDK\\include' -DNDEBUG -fo lefty.res ws/mswin32/lefty.rc
else
lefty.res : ws/mswin32/lefty.rc
	windres -l 0x409 -Iws/mswin32 -DNDEBUG -Ocoff -o lefty.res ws/mswin32/lefty.rc
endif

else
lefty_LDADD = $(top_builddir)/cmd/lefty/dot2l/libdot2l.la \
	$(top_builddir)/cmd/lefty/ws/x11/libws.la \
	$(top_builddir)/cmd/lefty/ws/x11/libfilereq/libfilereq.la \
	$(top_builddir)/cmd/lefty/os/unix/libos.la \
	@X_LIBS@ @XAW_LIBS@ @X_PRE_LIBS@ @XPM_LIBS@ \
	-lXt -lXmu -lXext -lX11 @SOCKET_LIBS@ @MATH_LIBS@
endif

lefty.1.pdf: $(srcdir)/lefty.1
	- @GROFF@ -Tps -man $(srcdir)/lefty.1 | @PS2PDF@ - - >lefty.1.pdf

EXTRA_DIST = $(lefty_DATA) $(man) $(pdf) \
	lefty.vcproj colors.txt Makefile.old aix_mods cs2l/*

DISTCLEANFILES = $(pdf)
