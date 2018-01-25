# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

if WITH_IPSEPCOLA
noinst_LTLIBRARIES = libvpsc_C.la
endif

noinst_HEADERS = \
	solve_VPSC.h block.h blocks.h variable.h constraint.h \
	generate-constraints.h remove_rectangle_overlap.h \
	solve_VPSC.h csolve_VPSC.h pairingheap/PairingHeap.h \
	pairingheap/dsexceptions.h

libvpsc_C_la_SOURCES = \
	block.cpp blocks.cpp constraint.cpp generate-constraints.cpp \
	pairingheap/PairingHeap.cpp remove_rectangle_overlap.cpp \
	solve_VPSC.cpp csolve_VPSC.cpp variable.cpp

EXTRA_DIST = Makefile.old vpsc.vcproj

