# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
        -I$(top_srcdir) \
	-I$(top_srcdir)/lib/common \
	-I$(top_srcdir)/lib/cgraph \
	-I$(top_srcdir)/lib/cdt 

noinst_HEADERS = SparseMatrix.h general.h BinaryHeap.h IntStack.h vector.h DotIO.h \
    LinkedList.h colorutil.h color_palette.h mq.h clustering.h

noinst_LTLIBRARIES = libsparse_C.la

libsparse_C_la_SOURCES = SparseMatrix.c general.c BinaryHeap.c IntStack.c vector.c DotIO.c \
    LinkedList.c colorutil.c mq.c clustering.c

EXTRA_DIST = Makefile.old gvsparse.vcproj
