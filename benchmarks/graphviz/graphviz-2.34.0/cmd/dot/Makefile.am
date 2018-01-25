# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

pdfdir = $(pkgdatadir)/doc/pdf

linkedman = neato.1 twopi.1 fdp.1 circo.1
linkedpdf = neato.1.pdf twopi.1.pdf fdp.1.pdf circo.1.pdf
linkedprogram = neato$(EXEEXT) twopi$(EXEEXT) fdp$(EXEEXT) circo$(EXEEXT) \
    osage$(EXEEXT) patchwork$(EXEEXT)

if WITH_SFDP
linkedman += sfdp.1
linkedpdf += sfdp.1.pdf
linkedprogram += sfdp$(EXEEXT)
endif

AM_CPPFLAGS = \
        -I$(top_srcdir)/lib/common \
        -I$(top_srcdir)/lib/gvc \
	-I$(top_srcdir)/lib/pathplan \
	-I$(top_srcdir)/lib/$(GRAPH) \
	-I$(top_srcdir)/lib/cdt

if ENABLE_SHARED
if ENABLE_LTDL
if ENABLE_STATIC
bin_PROGRAMS = dot dot_static
else
bin_PROGRAMS = dot dot_builtins
endif
else
if ENABLE_STATIC
bin_PROGRAMS = dot_static dot_builtins
else
bin_PROGRAMS = dot_builtins
endif
endif
else
if ENABLE_STATIC
bin_PROGRAMS = dot_static
endif
endif

man_MANS = dot.1 osage.1 patchwork.1
pdf_DATA = dot.1.pdf osage.1.pdf patchwork.1.pdf

dot_SOURCES = dot.c no_builtins.c
dot_CPPFLAGS = $(AM_CPPFLAGS) -DDEMAND_LOADING=1
dot_LDADD = \
	$(top_builddir)/lib/gvc/libgvc.la \
	$(top_builddir)/lib/$(GRAPH)/lib$(GRAPH).la

install-data-hook:
	(cd $(DESTDIR)$(man1dir); for i in $(linkedman); do rm -f $$i; $(LN_S) dot.1 $$i; done;)
	(cd $(DESTDIR)$(pdfdir); for i in $(linkedpdf); do rm -f $$i; $(LN_S) dot.1.pdf $$i; done;)

# create "neato", "fdp" etc as soflinks to "dot"
# run "dot -c", if possible, to create plugin config
install-exec-hook:
	(cd $(DESTDIR)$(bindir); if test -x dot$(EXEEXT); then for i in $(linkedprogram); do rm -f $$i; $(LN_S) dot$(EXEEXT) $$i; done; fi;)
	if test "x$(DESTDIR)" = "x"; then if test -x $(bindir)/dot$(EXEEXT); then if test -x /sbin/ldconfig; then /sbin/ldconfig 2>/dev/null; fi; $(bindir)/dot$(EXEEXT) -c; else $(bindir)/dot_static$(EXEEXT) -c; fi; fi

uninstall-hook:
	(cd $(DESTDIR)$(man1dir); for i in $(linkedman); do rm -f $$i; done;)
	(cd $(DESTDIR)$(pdfdir); for i in $(linkedpdf); do rm -f $$i; done;)
	(cd $(DESTDIR)$(bindir); for i in $(linkedprogram); do rm -f $$i; done;)
	(cd $(DESTDIR)$(libdir); rm -f config;)

dot.1.pdf: $(srcdir)/dot.1
	- $(GROFF) -Tps -man $(srcdir)/dot.1 | $(PS2PDF) - - >dot.1.pdf

osage.1.pdf: $(srcdir)/osage.1
	- $(GROFF) -Tps -man $(srcdir)/osage.1 | $(PS2PDF) - - >osage.1.pdf

patchwork.1.pdf: $(srcdir)/patchwork.1
	- $(GROFF) -Tps -man $(srcdir)/patchwork.1 | $(PS2PDF) - - >patchwork.1.pdf

dot_static_SOURCES = dot.c dot_builtins.c
dot_static_CPPFLAGS = $(AM_CPPFLAGS) -DDEMAND_LOADING=0
dot_static_LDADD = \
	$(top_builddir)/plugin/dot_layout/libgvplugin_dot_layout_C.la \
	$(top_builddir)/plugin/neato_layout/libgvplugin_neato_layout_C.la \
	$(top_builddir)/plugin/core/libgvplugin_core_C.la \
	$(top_builddir)/lib/gvc/libgvc_C.la \
	$(top_builddir)/lib/pathplan/libpathplan_C.la \
	$(top_builddir)/lib/$(GRAPH)/lib$(GRAPH)_C.la \
	$(top_builddir)/lib/xdot/libxdot_C.la \
	$(top_builddir)/lib/cdt/libcdt_C.la \
       	$(PANGOCAIRO_LIBS) $(PANGOFT2_LIBS) $(GTS_LIBS) $(EXPAT_LIBS) $(Z_LIBS) $(LIBGEN_LIBS) $(SOCKET_LIBS) $(IPSEPCOLA_LIBS) $(MATH_LIBS)

dot_builtins_SOURCES = dot.c dot_builtins.c
dot_builtins_CPPFLAGS = $(AM_CPPFLAGS) -DDEMAND_LOADING=1
dot_builtins_LDADD = \
	$(top_builddir)/plugin/dot_layout/libgvplugin_dot_layout.la \
	$(top_builddir)/plugin/neato_layout/libgvplugin_neato_layout.la \
	$(top_builddir)/plugin/core/libgvplugin_core.la \
	$(top_builddir)/lib/gvc/libgvc.la \
	$(top_builddir)/lib/pathplan/libpathplan.la \
	$(top_builddir)/lib/$(GRAPH)/lib$(GRAPH).la \
	$(top_builddir)/lib/xdot/libxdot.la \
	$(top_builddir)/lib/cdt/libcdt.la \
       	$(GTS_LIBS) $(EXPAT_LIBS) $(Z_LIBS) $(LIBGEN_LIBS) $(SOCKET_LIBS) $(IPSEPCOLA_LIBS) $(MATH_LIBS)

if WITH_LIBGD
dot_static_LDADD += $(top_builddir)/plugin/gd/.libs/libgvplugin_gd_C.a $(GD_LIBS)
dot_builtins_LDADD += $(top_builddir)/plugin/gd/libgvplugin_gd.la $(GD_LIBS)
endif

if WITH_PANGOCAIRO
if WITH_WEBP
dot_static_LDADD += $(top_builddir)/plugin/webp/.libs/libgvplugin_webp_C.a $(WEBP_LIBS)
dot_builtins_LDADD += $(top_builddir)/plugin/webp/libgvplugin_webp.la $(WEBP_LIBS)
endif
dot_static_LDADD += $(top_builddir)/plugin/pango/.libs/libgvplugin_pango_C.a $(PANGOCAIRO_LIBS)
dot_builtins_LDADD += $(top_builddir)/plugin/pango/libgvplugin_pango.la $(PANGOCAIRO_LIBS)
if WITH_WEBP
dot_static_LDADD += $(top_builddir)/plugin/webp/.libs/libgvplugin_webp_C.a $(WEBP_LIBS)
dot_builtins_LDADD += $(top_builddir)/plugin/webp/libgvplugin_webp.la $(WEBP_LIBS)
endif
endif

if WITH_LASI
dot_static_LDADD += $(top_builddir)/plugin/lasi/.libs/libgvplugin_lasi_C.a $(LASI_LIBS) -lstdc++
dot_builtins_LDADD += $(top_builddir)/plugin/lasi/libgvplugin_lasi.la $(LASI_LIBS) -lstdc++
endif

if WITH_LASI
dot_static_LDADD += $(top_builddir)/plugin/lasi/.libs/libgvplugin_lasi_C.a $(LASI_LIBS) -lstdc++
dot_builtins_LDADD += $(top_builddir)/plugin/lasi/libgvplugin_lasi.la $(LASI_LIBS) -lstdc++
endif


EXTRA_DIST = Makefile.old dot.1 dot.1.pdf osage.1 osage.1.pdf patchwork.1 patchwork.1.pdf dot.vcproj

DISTCLEANFILES = dot.1.pdf osage.1.pdf
