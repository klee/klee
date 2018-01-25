# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

GRAPHS = ER.gv ngk10_4.gv process.gv Heawood.gv Petersen.gv

graphdir = $(pkgdatadir)/graphs
undirecteddir = $(graphdir)/undirected
undirected_DATA = $(GRAPHS)

EXTRA_DIST = $(GRAPHS)

CLEANFILES = core *.gv.* *.png .xvpics/* 

DOT = $(top_builddir)/cmd/dot/dot_builtins

test:
	$(DOT) -Kneato \
		-Tcanon -Tgv -Tplain -Tplain-ext -Txdot \
		-Tcmap -Tcmapx -Tismap -Timap \
		-Tjpg -Tpdf -Tfig -Tgif -Tpng \
		-Tps -Tps2 -Tps:lasi -Tps:cairo -Tpdf \
		-Tsvg -Tsvg:cairo -Tsvgz -Tvrml \
		-O $(GRAPHS) 

canon:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tcanon -o$$i.canon $(top_srcdir)/graphs/undirected/$$i; done

cmap:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tcmap -o$$i.cmap $(top_srcdir)/graphs/undirected/$$i; done

cmapx:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tcmapx -o$$i.cmapx $(top_srcdir)/graphs/undirected/$$i; done

dia:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tdia -o$$i.dia $(top_srcdir)/graphs/undirected/$$i; done

fig:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tfig -o$$i.fig $(top_srcdir)/graphs/undirected/$$i; done

gd:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tgd -o$$i.gd $(top_srcdir)/graphs/undirected/$$i; done

gd2:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tgd2 -o$$i.gd2 $(top_srcdir)/graphs/undirected/$$i; done

gif:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tgif -o$$i.gif $(top_srcdir)/graphs/undirected/$$i; done

gv dot:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tgv -o$$i.gv $(top_srcdir)/graphs/undirected/$$i; done

hpgl:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Thpgl -o$$i.hpgl $(top_srcdir)/graphs/undirected/$$i; done

ismap:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tismap -o$$i.ismap $(top_srcdir)/graphs/undirected/$$i; done

imap:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Timap -o$$i.imap $(top_srcdir)/graphs/undirected/$$i; done

jpg:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tjpg -o$$i.jpg $(top_srcdir)/graphs/undirected/$$i; done

mif:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tmif -o$$i.mif $(top_srcdir)/graphs/undirected/$$i; done

mp:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tmp -o$$i.mp $(top_srcdir)/graphs/undirected/$$i; done

pcl:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tpcl -o$$i.pcl $(top_srcdir)/graphs/undirected/$$i; done

pic:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tpic -o$$i.pic $(top_srcdir)/graphs/undirected/$$i; done

plain:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tplain -o$$i.plain $(top_srcdir)/graphs/undirected/$$i; done

plain-ext:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tplain-ext -o$$i.plain-ext $(top_srcdir)/graphs/undirected/$$i; done

png:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tpng -o$$i.png $(top_srcdir)/graphs/undirected/$$i; done

ps:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tps -o$$i.ps $(top_srcdir)/graphs/undirected/$$i; done

ps_cairo:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tps:cairo -o$$i.cairo.ps $(top_srcdir)/graphs/undirected/$$i; done

ps_lasi:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tps:lasi -o$$i.lasi.ps $(top_srcdir)/graphs/undirected/$$i; done

ps2:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tps2 -o$$i.ps2 $(top_srcdir)/graphs/undirected/$$i; done

svg:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tsvg -o$$i.svg $(top_srcdir)/graphs/undirected/$$i; done

svgz:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tsvgz -o$$i.svgz $(top_srcdir)/graphs/undirected/$$i; done

wbmp:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Twbmp -o$$i.wbmp $(top_srcdir)/graphs/undirected/$$i; done

vrml:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tvrml -o$$i.vrml $(top_srcdir)/graphs/undirected/$$i; done

vtx:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Tvtx -o$$i.vtx $(top_srcdir)/graphs/undirected/$$i; done

xdot:
	for i in $(GRAPHS); do echo "neato $$i"; \
		$(DOT) -Kneato -Txdot -o$$i.xdot $(top_srcdir)/graphs/undirected/$$i; done
