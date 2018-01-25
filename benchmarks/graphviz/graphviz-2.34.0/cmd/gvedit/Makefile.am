# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_CGRAPH
GRAPH = cgraph
else
GRAPH = graph
endif

SUBDIRS = ui images

pdfdir = $(pkgdatadir)/doc/pdf
gveditdir = $(pkgdatadir)/gvedit

if WITH_QT
man_MANS = gvedit.1
pdf_DATA = gvedit.1.pdf
bin_PROGRAMS = gvedit
gvedit_DATA = attrs.txt
endif

AM_CPPFLAGS = \
        -I$(top_srcdir)/lib/common \
        -I$(top_srcdir)/lib/gvc \
        -I$(top_srcdir)/lib/pathplan \
        -I$(top_srcdir)/lib/$(GRAPH) \
        -I$(top_srcdir)/lib/cdt

gvedit_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(QTGUI_CFLAGS) \
	$(QTCORE_CFLAGS) \
	-DDEMAND_LOADING=1 \
	-DHAVE_CONFIG_H \
	-DGVEDIT_DATADIR=\""$(pkgdatadir)/gvedit"\"

gvedit_LDADD = \
	$(top_builddir)/lib/gvc/libgvc.la \
	$(QTGUI_LIBS) \
	$(QTCORE_LIBS) \
	$(EXPAT_LIBS) $(Z_LIBS) 

noinst_HEADERS = csettings.h imageviewer.h mainwindow.h mdichild.h ui_settings.h 

MOCABLES = moc_csettings.cpp moc_imageviewer.cpp moc_mainwindow.cpp moc_mdichild.cpp qrc_mdi.cpp

gvedit_SOURCES = main.cpp csettings.cpp mainwindow.cpp imageviewer.cpp mdichild.cpp $(MOCABLES)

$(MOCABLES): mocables

.PHONY: mocables
mocables: qMakefile
	rm -f $(MOCABLES) # don't use distributed mocables poss from diff version of Qt
	$(MAKE) -f qMakefile prefix=${prefix} exec_prefix=${exec_prefix} mocables compiler_rcc_make_all

qMakefile: gvedit.pro
	$(QMAKE) -o qMakefile gvedit.pro

attrs.txt: $(top_srcdir)/doc/infosrc/attrs
	cp $(top_srcdir)/doc/infosrc/attrs attrs.txt

gvedit.1.pdf: $(srcdir)/gvedit.1
	- $(GROFF) -Tps -man $(srcdir)/gvedit.1 | @PS2PDF@ - - >gvedit.1.pdf

EXTRA_DIST = gvedit.vcproj gvedit.pro.in mdi.qrc $(man_MANS) $(pdf_DATA)

CLEANFILES = moc_csettings.cpp moc_imageviewer.cpp moc_mainwindow.cpp moc_mdichild.cpp qrc_mdi.cpp
	
DISTCLEANFILES = attrs.txt $(pdf_DATA) qMakefile

clean-local:
	-$(MAKE) -f qMakefile clean

distclean-local:
	-$(MAKE) -f qMakefile distclean

