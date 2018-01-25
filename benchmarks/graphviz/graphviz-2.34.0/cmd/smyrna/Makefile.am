# $Id$Revision$
## Process this file with automake to produce Makefile.in

pdfdir = $(pkgdatadir)/doc/pdf


man = smyrna.1
pdf = smyrna.1.pdf


AM_CPPFLAGS = \
	-DSMYRNA_PATH=\""$(pkgdatadir)/smyrna"\" \
        -I$(top_srcdir) \
        -I$(top_srcdir)/lib/cgraph \
        -I$(top_srcdir)/lib/cdt \
        -I$(top_srcdir)/lib/glcomp \
        -I$(top_srcdir)/lib/utilities \
        -I$(top_srcdir)/lib/xdot \
        -I$(top_srcdir)/lib/glcomp \
        -I$(top_srcdir)/lib/ast \
        -I$(top_srcdir)/lib/sfio \
        -I$(top_srcdir)/lib/neatogen \
        -I$(top_srcdir)/lib/topfish \
        -I$(top_srcdir)/lib/gvpr \
        -I$(top_srcdir)/lib/common \
        -I$(top_srcdir)/cmd/smyrna/gui \
	$(GTK_CFLAGS) $(GLUT_CFLAGS) $(GTKGLEXT_CFLAGS) $(GLADE_CFLAGS) $(FREETYPE2_CFLAGS) $(FONTCONFIG_CFLAGS) $(GTS_CFLAGS)

man_MANS =
pdf_DATA =
bin_PROGRAMS =
if WITH_SMYRNA
man_MANS += $(man)
pdf_DATA += $(pdf)
if ENABLE_SHARED
bin_PROGRAMS += smyrna 
endif
if ENABLE_STATIC
bin_PROGRAMS += smyrna_static
endif
endif

noinst_HEADERS = arcball.h draw.h glexpose.h \
	glmotion.h gltemplate.h gui/appmouse.h gui/callbacks.h \
	hotkeymap.h materials.h md5.h polytess.h selectionfuncs.h \
	smyrna_utils.h smyrnadefs.h topfisheyeview.h \
	topviewdefs.h topviewfuncs.h trackball.h tvnodes.h \
	viewport.h viewportcamera.h support.h \
	gui/datalistcallbacks.h gui/frmobjectui.h \
	gui/glcompui.h gui/gui.h gui/menucallbacks.h \
	gui/toolboxcallbacks.h gui/topviewsettings.h gvprpipe.h hier.h glutrender.h

smyrna_SOURCES = arcball.c draw.c glexpose.c \
	glmotion.c gltemplate.c gui/appmouse.c gui/callbacks.c \
	gvprpipe.c hier.c hotkeymap.c main.c md5.c polytess.c \
	selectionfuncs.c smyrna_utils.c topfisheyeview.c \
	topviewfuncs.c trackball.c  tvnodes.c \
	viewport.c viewportcamera.c \
	gui/datalistcallbacks.c gui/frmobjectui.c \
	gui/glcompui.c gui/gui.c gui/menucallbacks.c \
	gui/toolboxcallbacks.c gui/topviewsettings.c glutrender.c

smyrna_LDADD = $(top_builddir)/lib/cgraph/libcgraph_C.la \
		$(top_builddir)/lib/cdt/libcdt_C.la \
		$(top_builddir)/lib/xdot/libxdot_C.la \
		$(top_builddir)/lib/glcomp/libglcomp_C.la \
		$(top_builddir)/lib/topfish/libtopfish_C.la \
		$(top_builddir)/lib/common/libcommon_C.la \
		$(top_builddir)/lib/gvpr/libgvpr_C.la \
		$(top_builddir)/lib/expr/libexpr_C.la \
		$(top_builddir)/lib/ingraphs/libingraphs_C.la \
		$(top_builddir)/lib/neatogen/libneatogen_C.la \
		$(GTK_LIBS) $(GLUT_LIBS) $(GTKGLEXT_LIBS) $(GLADE_LIBS) $(EXPAT_LIBS) $(GTS_LIBS) $(EXTRA_SMYRNA_LDFLAGS)

smyrna_static_SOURCES = $(smyrna_SOURCES)
smyrna_static_LDADD = $(top_builddir)/lib/cgraph/libcgraph_C.la \
		$(top_builddir)/lib/cdt/libcdt_C.la \
		$(top_builddir)/lib/xdot/libxdot_C.la \
		$(top_builddir)/lib/glcomp/libglcomp_C.la \
		$(top_builddir)/lib/topfish/libtopfish_C.la \
		$(top_builddir)/lib/common/libcommon_C.la \
		$(top_builddir)/lib/gvpr/libgvpr_C.la \
		$(top_builddir)/lib/expr/libexpr_C.la \
		$(top_builddir)/lib/ingraphs/libingraphs_C.la \
		$(top_builddir)/lib/neatogen/libneatogen_C.la \
		$(GTK_LIBS) $(GLUT_LIBS) $(GTKGLEXT_LIBS) $(GLADE_LIBS) $(EXPAT_LIBS) $(GTS_LIBS)

smyrna.1.pdf: $(srcdir)/smyrna.1
	- @GROFF@ -Tps -man $(srcdir)/smyrna.1 | @PS2PDF@ - - >smyrna.1.pdf

EXTRA_DIST = smyrna.vcproj $(man) $(pdf)

DISTCLEANFILES = $(pdf)

