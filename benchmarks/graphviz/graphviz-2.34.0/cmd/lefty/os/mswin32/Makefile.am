# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/cmd/lefty

if WITH_WIN32
noinst_LTLIBRARIES = libos.la
endif

libos_la_SOURCES = io.c

EXTRA_DIST = $(libos_la_SOURCES)
