# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

EXPR_VERSION="4:0:0"

pdfdir = $(pkgdatadir)/doc/pdf
pkgconfigdir = $(libdir)/pkgconfig

AM_CPPFLAGS = \
        -I. \
        -I$(top_srcdir) \
        -I$(top_srcdir)/lib/vmalloc \
	-I$(top_srcdir)/lib/sfio \
	-I$(top_srcdir)/lib/ast \
	-I$(top_srcdir)/lib/cdt

noinst_HEADERS = exgram.h exlib.h expr.h
noinst_LTLIBRARIES = libexpr_C.la

### expr.h includes ast.h and vmalloc.h which we don't install
#lib_LTLIBRARIES = libexpr.la
man_MANS = expr.3
pdf_DATA = expr.3.pdf
#pkgconfig_DATA = libexpr.pc

#### -no-undefined breaks Mac OS/X builds.
#### libast has "extern char **environ" which remains
####      undefined until linked with main()
#libexpr_la_LDFLAGS = -version-info $(EXPR_VERSION) -no-undefined

libexpr_C_la_SOURCES = excc.c excontext.c exdata.c exerror.c \
	exeval.c exexpr.c exlexname.c exopen.c exrewind.c extoken.c \
	extype.c exzero.c exparse.y exnospace.c exstash.c 
libexpr_C_la_LIBADD = \
	$(top_builddir)/lib/ast/libast_C.la \
	$(top_builddir)/lib/vmalloc/libvmalloc_C.la \
	$(top_builddir)/lib/sfio/libsfio_C.la \
	$(top_builddir)/lib/cdt/libcdt.la

#libexpr_la_LDFLAGS = -version-info $(EXPR_VERSION)
#libexpr_la_SOURCES = $(libexpr_C_la_SOURCES)
#libexpr_la_LIBADD = $(libexpr_C_la_LIBADD)

$(libexpr_C_la_OBJECTS) $(libexpr_la_OBJECTS): \
	exparse.h exparse.c exgram.h exop.h

exop.h: exparse.h
	echo "static const char* exop[] = {" > exop.h
	echo "	\"MINTOKEN\"," >> exop.h
	$(SED) -e '1,/^[   ]*#[    ]*define[   ][  ]*MINTOKEN/d' -e '/MAXTOKEN/,$$d' -e '/^[ 	]*#[ 	]*define[ 	][ 	]*[A-Z]/!d' -e 's/^[ 	]*#[ 	]*define[ 	]*\([A-Z0-9_]*\).*/	"\1",/' < exparse.h >> exop.h
	echo "};" >> exop.h

exparse.c: y.tab.c 
	@SED@ "s/yy/ex/g" <y.tab.c | \
		@SED@ "s/YY/EX/g" | \
		@SED@ "s/fprintf/sfprintf/g" | \
		@SED@ "s/FILE/Sfio_t/g" | \
		@SED@ "s/stderr/sfstderr/g" > exparse.c

exparse.h: y.tab.h
	echo "#ifndef _EXPARSE_H" > exparse.h
	echo "#define _EXPARSE_H" >> exparse.h
	@SED@ "s/yy/ex/g" < y.tab.h | @SED@ "s/YY/EX/g" >> exparse.h
	echo "#endif /* _EXPARSE_H */" >> exparse.h

y.tab.c y.tab.h: y.output

y.output: $(top_srcdir)/lib/expr/exparse.y
	@YACC@ -dtv $(top_srcdir)/lib/expr/exparse.y

expr.3.pdf: $(srcdir)/expr.3
	- @GROFF@ -Tps -man $(srcdir)/expr.3 | @PS2PDF@ - - >expr.3.pdf

EXTRA_DIST = $(man_MANS) $(pdf_DATA) \
	Makefile.nmake Makefile.orig Makefile.old \
	exparse.c exparse.h exop.h \
	y.tab.c y.tab.h y.output expr.vcproj

DISTCLEANFILES = $(pdf_DATA) exparse.[ch] exop.h y.tab.[ch] y.output
