# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

SUBDIRS = Sfio_f

AM_CPPFLAGS = -I$(top_srcdir)/lib/ast

AM_CFLAGS = -Dvt_threaded=0

noinst_HEADERS = sfhdr.h sfio.h sfio_t.h vthread.h
noinst_LTLIBRARIES = libsfio_C.la

libsfio_C_la_SOURCES = sfclose.c sfclrlock.c sfcvt.c sfdisc.c \
	sfdlen.c sfexcept.c sfexit.c sfextern.c sffcvt.c sffilbuf.c \
	sfflsbuf.c sfgetd.c sfgetl.c sfgetm.c sfgetr.c sfgetu.c \
	sfllen.c sfmode.c sfmove.c sfmutex.c sfnew.c sfnotify.c \
	sfnputc.c sfopen.c sfpkrd.c sfpoll.c sfpool.c sfpopen.c \
	sfprintf.c sfprints.c sfpurge.c sfputd.c sfputl.c sfputm.c \
	sfputr.c sfputu.c sfraise.c sfrd.c sfread.c sfreserve.c \
	sfresize.c sfscanf.c sfseek.c sfset.c sfsetbuf.c sfsetfd.c \
	sfsize.c sfsk.c sfstack.c sfstrtod.c sfswap.c sfsync.c \
	sftable.c sftell.c sftmp.c sfungetc.c sfvprintf.c \
	sfvscanf.c sfwr.c sfwrite.c

libsfio_C_la_DEPENDENCIES = \
	Sfio_f/libsfiof_C.la

$(top_builddir)/FEATURE/sfio: $(top_srcdir)/lib/sfio/features/sfio
	mkdir -p $(top_builddir)/FEATURE
	$(top_srcdir)/iffe - set cc $(CC) $(CCMODE) $(CXFLAGS) : run $(top_srcdir)/lib/sfio/features/sfio >$@

$(libsfio_la_OBJECTS): $(top_builddir)/FEATURE/sfio

DISTCLEANFILES = $(top_builddir)/FEATURE/sfio

EXTRA_DIST = README Makefile.old Makefile.orig features sfio.vcproj
