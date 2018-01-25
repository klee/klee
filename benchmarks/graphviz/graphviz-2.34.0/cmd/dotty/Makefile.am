# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

man = dotty.1
pdf = dotty.1.pdf

leftydir = $(pkgdatadir)/lefty
pdfdir = $(pkgdatadir)/doc/pdf

if WITH_X
man_MANS = $(man)
pdf_DATA = $(pdf)
if WITH_WIN32
bin_PROGRAMS = dotty
else
bin_SCRIPTS = dotty
endif
lefty_DATA = dotty.lefty dotty_draw.lefty dotty_edit.lefty \
        dotty_layout.lefty dotty_ui.lefty
endif

if WITH_WIN32
dotty_SOURCES = mswin32/dotty.c
dotty_LDADD = -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32
else
dotty: $(top_srcdir)/cmd/dotty/dotty.sh
	cp $(top_srcdir)/cmd/dotty/dotty.sh dotty
	chmod +x dotty
endif

dotty.1.pdf: $(srcdir)/dotty.1
	- @GROFF@ -Tps -man $(srcdir)/dotty.1 | @PS2PDF@ - - >dotty.1.pdf

EXTRA_DIST = $(lefty_DATA) $(man) $(pdf) \
	dotty.bsh dotty.ksh dotty.sh \
	Makefile.old notes mswin32 dotty.vcproj

DISTCLEANFILES = $(pdf) dotty
