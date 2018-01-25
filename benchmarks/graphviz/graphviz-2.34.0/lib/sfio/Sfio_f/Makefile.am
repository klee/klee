# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/lib/sfio

AM_CFLAGS = -Dvt_threaded=0

noinst_LTLIBRARIES = libsfiof_C.la

libsfiof_C_la_SOURCES = _sfclrerr.c _sfdlen.c _sfeof.c _sferror.c \
	_sffileno.c _sfgetc.c _sfllen.c _sfputc.c _sfputd.c \
	_sfputl.c _sfputm.c _sfputu.c _sfslen.c _sfstacked.c \
	_sfulen.c _sfvalue.c

${top_builddir}/FEATURE/sfio:  ${top_srcdir}/lib/sfio/features/sfio
	mkdir -p ${top_builddir}/FEATURE
	$(top_srcdir)/iffe - set cc $(CC) $(CCMODE) $(CXFLAGS) : run ${top_srcdir}/lib/sfio/features/sfio > $@

$(libsfiof_C_la_OBJECTS): ${top_builddir}/FEATURE/sfio

DISTCLEANFILES = ${top_builddir}/FEATURE/sfio

EXTRA_DIST = Makefile.old
