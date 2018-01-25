# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

# SUBDIRS = dot neato lefty dotty info
SUBDIRS = info schema

pdf = dotguide.pdf dottyguide.pdf leftyguide.pdf neatoguide.pdf libgraph/Agraph.pdf libguide/libguide.pdf smyrna/smyrna.pdf
html = build.html FAQ.html index.html winbuild.html pspdf.png char.html gdtclft.entities.example.png todo.html
txt = Dot.ref latex_suggestions.txt fontfaq.txt addingLayout.txt

pdfdir = $(pkgdatadir)/doc/pdf
htmldir = $(pkgdatadir)/doc/html
txtdir = $(pkgdatadir)/doc

pdf_DATA = $(pdf)
html_DATA = $(html)
txt_DATA = $(txt)

EXTRA_DIST = $(txt) $(pdf) $(html) schema/*.xml schema/*.xslt infosrc/*
