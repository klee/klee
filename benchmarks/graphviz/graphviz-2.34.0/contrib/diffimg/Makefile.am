# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

VERSION=0.2

AM_CPPFLAGS = @GD_INCLUDES@

pdfdir = $(pkgdatadir)/doc/pdf

man_MANS = diffimg.1
pdf_DATA = diffimg.1.pdf

if WITH_LIBGD
bin_PROGRAMS = diffimg
endif

diffimg_SOURCES = diffimg.c

diffimg_LDADD = @GD_LIBS@ @MATH_LIBS@

diffimg.1.pdf: $(srcdir)/diffimg.1
	- $(GROFF) -Tps -man $(srcdir)/diffimg.1 | $(PS2PDF) - - >diffimg.1.pdf

GRAPH = "digraph G { hello -> world }"

test: diffimg
	echo $(GRAPH) | dot -Tpng:cg >hello1.png
	echo $(GRAPH) | dot -Tpng:cg >hello2.png
	if `./diffimg hello1.png hello2.png >test1.png`;then echo same;else echo different;fi
	echo $(GRAPH) | dot -Grankdir=LR -Tpng:cg >hello2.png
	if `./diffimg hello1.png hello2.png >test2.png`;then echo same;else echo different;fi

CLEANFILES = *.png
EXTRA_DIST = diffimg.vcproj diffimg.1 diffimg.1.pdf
DISTCLEANFILES = diffimg.1.pdf
