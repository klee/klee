# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

GRAPH_VERSION="5:0:0"

pdfdir = $(pkgdatadir)/doc/pdf
pkgconfigdir = $(libdir)/pkgconfig

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib/cdt

man_MANS = graph.3
if WITH_CGRAPH
else
pkginclude_HEADERS = graph.h
noinst_HEADERS = libgraph.h triefa.h parser.h agxbuf.h
noinst_LTLIBRARIES = libgraph_C.la
lib_LTLIBRARIES = libgraph.la
pkgconfig_DATA = libgraph.pc
pdf_DATA = graph.3.pdf
endif

libgraph_C_la_SOURCES = \
	attribs.c agxbuf.c edge.c graph.c graphio.c lexer.c \
	node.c parser.y refstr.c trie.c
 
libgraph_la_LDFLAGS = -version-info $(GRAPH_VERSION) -no-undefined
libgraph_la_SOURCES = $(libgraph_C_la_SOURCES)
libgraph_la_LIBADD = $(top_builddir)/lib/cdt/libcdt.la

lexer.o lexer.lo trie.o trie.lo: parser.h

parser.c: y.tab.c
	@SED@ "s/yy/ag/g" < y.tab.c > parser.c

parser.h: y.tab.h
	@SED@ "s/yy/ag/g" < y.tab.h > parser.h

y.tab.c y.tab.h: y.output

y.output: $(top_srcdir)/lib/graph/parser.y
	@YACC@ -dv $(top_srcdir)/lib/graph/parser.y

# this is a home-made tool
# trie.c : trie_input
#       triegen  < trie_input > trie.c

graph.3.pdf: $(srcdir)/graph.3
	- @GROFF@ -Tps -man $(srcdir)/graph.3 | @PS2PDF@ - - > graph.3.pdf

EXTRA_DIST = $(man_MANS) $(pdf_DATA) triefa.cP \
	Makefile.old parser.c parser.h \
	y.tab.c y.tab.h y.output graph.vcproj graph.def

DISTCLEANFILES = y.output parser.c parser.h y.tab.[ch] $(pdf_DATA)
