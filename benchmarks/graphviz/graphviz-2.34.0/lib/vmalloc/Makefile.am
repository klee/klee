# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

noinst_HEADERS = vmalloc.h vmhdr.h
noinst_LTLIBRARIES = libvmalloc_C.la

libvmalloc_C_la_SOURCES = malloc.c vmbest.c vmclear.c vmclose.c vmdcheap.c \
	vmdebug.c vmdisc.c vmexit.c vmlast.c vmopen.c vmpool.c vmprivate.c \
	vmprofile.c vmregion.c vmsegment.c vmset.c vmstat.c vmstrdup.c \
	vmtrace.c vmwalk.c

${top_builddir}/FEATURE/vmalloc: ${top_srcdir}/lib/vmalloc/features/vmalloc
	mkdir -p ${top_builddir}/FEATURE
	${top_srcdir}/iffe - set cc $(CC) $(CCMODE) $(CXFLAGS) : run ${top_srcdir}/lib/vmalloc/features/vmalloc > $@

$(libvmalloc_C_la_OBJECTS): ${top_builddir}/FEATURE/vmalloc

DISTCLEANFILES = ${top_builddir}/FEATURE/vmalloc

EXTRA_DIST = README Makefile.old features vmalloc.vcproj
