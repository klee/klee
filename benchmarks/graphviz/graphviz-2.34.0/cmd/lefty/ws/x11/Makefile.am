# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

SUBDIRS = libfilereq

AM_CPPFLAGS = -I$(top_srcdir)/cmd/lefty \
	-I$(top_srcdir)/cmd/lefty/ws/x11/libfilereq \
	-DFEATURE_X11 @XAW_INCLUDES@ @X_CFLAGS@

noinst_HEADERS = gcommon.h
if WITH_X
if WITH_XAW
noinst_LTLIBRARIES = libws.la
endif
endif

libws_la_SOURCES = garray.c gbutton.c gcanvas.c gcommon.c glabel.c \
	gmenu.c gpcanvas.c gquery.c gscroll.c gtext.c gview.c
