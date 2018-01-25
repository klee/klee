# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)

noinst_HEADERS = misc.h red_black_tree.h stack.h
noinst_LTLIBRARIES = librbtree_C.la

librbtree_C_la_SOURCES = misc.c red_black_tree.c stack.c

CLEANFILES = inkpot_lib_*.dat

EXTRA_DIST = LICENSE CHANGES makefile.txt test_red_black_tree.c \
	     simple_test.sh test_rb rbtree.vcproj

DISTCLEANFILES = 

