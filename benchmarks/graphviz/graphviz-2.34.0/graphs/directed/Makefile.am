# $Id$ $Revision$
## Process this file with automake to produce Makefile.in

GRAPHS = KW91.gv Latin1.gv NaN.gv abstract.gv alf.gv arrows.gv \
	awilliams.gv biological.gv clust.gv clust1.gv clust2.gv clust3.gv clust4.gv \
	clust5.gv crazy.gv ctext.gv dfa.gv fig6.gv fsm.gv \
	grammar.gv hashtable.gv honda-tokoro.gv jcctree.gv jsort.gv \
	ldbxtried.gv longflat.gv mike.gv nhg.gv oldarrows.gv \
	pgram.gv pm2way.gv pmpipe.gv polypoly.gv proc3d.gv psfonttest.gv \
	record2.gv records.gv rowe.gv japanese.gv russian.gv sdh.gv \
	shells.gv states.gv structs.gv table.gv train11.gv \
	trapeziumlr.gv tree.gv triedds.gv try.gv unix.gv unix2.gv \
	viewfile.gv world.gv switch.gv

graphdir = $(pkgdatadir)/graphs
directeddir = $(graphdir)/directed
directed_DATA = $(GRAPHS)

EXTRA_DIST = $(GRAPHS)

CLEANFILES = core *.gv.* *.png .xvpics/* 

DOT = $(top_builddir)/cmd/dot/dot_builtins

test:
	$(DOT) \
		-Tcanon -Tgv -Tplain -Tplain-ext -Txdot \
		-Tcmap -Tcmapx -Tismap -Timap \
		-Tjpg -Tpdf -Tfig -Tgif \
		-Tpng:cairo -Tpng:gd -Tpng:cairo:gd -Tpng:cairo:cairo -Tpng:gd:gd \
		-Tps -Tps2 -Tps:lasi -Tps:cairo -Tpdf \
		-Tsvg -Tsvg:cairo -Tsvgz -Tvrml \
		-O $(GRAPHS) 
ins_test:
	dot \
                -Tcanon -Tgv -Tplain -Tplain-ext -Txdot \
                -Tcmap -Tcmapx -Tismap -Timap \
                -Tjpg -Tpdf -Tfig -Tgif -Tpng \
		-Tpng:cairo -Tpng:gd -Tpng:cairo:gd -Tpng:cairo:cairo -Tpng:gd:gd \
                -Tps -Tps2 -Tps:lasi -Tps:cairo -Tpdf \
                -Tsvg -Tsvg:cairo -Tsvgz -Tvrml \
                -O $(GRAPHS) 

canon:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tcanon -o$$i.canon $(top_srcdir)/graphs/directed/$$i; done

cmap:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tcmap -o$$i.cmap $(top_srcdir)/graphs/directed/$$i; done
cmapx:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tcmapx -o$$i.cmapx $(top_srcdir)/graphs/directed/$$i; done

dia:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tdia -o$$i.dia $(top_srcdir)/graphs/directed/$$i; done

fig:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tfig -o$$i.fig $(top_srcdir)/graphs/directed/$$i; done

gd:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tgd -o$$i.gd $(top_srcdir)/graphs/directed/$$i; done

gd2:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tgd2 -o$$i.gd2 $(top_srcdir)/graphs/directed/$$i; done

gif:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tgif -o$$i.gif $(top_srcdir)/graphs/directed/$$i; done

gv dot:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tgv -o$$i.gv $(top_srcdir)/graphs/directed/$$i; done

hpgl:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Thpgl -o$$i.hpgl $(top_srcdir)/graphs/directed/$$i; done

ismap:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tismap -o$$i.ismap $(top_srcdir)/graphs/directed/$$i; done

imap:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Timap -o$$i.imap $(top_srcdir)/graphs/directed/$$i; done

jpg:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tjpg -o$$i.jpg $(top_srcdir)/graphs/directed/$$i; done

mif:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tmif -o$$i.mif $(top_srcdir)/graphs/directed/$$i; done

mp:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tmp -o$$i.mp $(top_srcdir)/graphs/directed/$$i; done

pcl:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tpcl -o$$i.pcl $(top_srcdir)/graphs/directed/$$i; done

pic:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tpic -o$$i.pic $(top_srcdir)/graphs/directed/$$i; done

plain:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tplain -o$$i.plain $(top_srcdir)/graphs/directed/$$i; done

plain-ext:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tplain-ext -o$$i.plain-ext $(top_srcdir)/graphs/directed/$$i; done

pdf:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tpdf -o$$i.pdf $(top_srcdir)/graphs/directed/$$i; done

png:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tpng -o$$i.png $(top_srcdir)/graphs/directed/$$i; done

png_cg:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tpng:cg -o$$i.cg.png $(top_srcdir)/graphs/directed/$$i; done

ps:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tps -o$$i.ps $(top_srcdir)/graphs/directed/$$i; done

ps_gd:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tps:gd -o$$i.gd.ps $(top_srcdir)/graphs/directed/$$i; done

ps_cairo:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tps:cairo -o$$i.cairo.ps $(top_srcdir)/graphs/directed/$$i; done

ps_lasi:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tps:lasi -o$$i.lasi.ps $(top_srcdir)/graphs/directed/$$i; done

ps2:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tps2 -o$$i.ps2 $(top_srcdir)/graphs/directed/$$i; done

svg:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tsvg -o$$i.svg $(top_srcdir)/graphs/directed/$$i; done

svgz:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tsvgz -o$$i.svgz $(top_srcdir)/graphs/directed/$$i; done

wbmp:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Twbmp -o$$i.wbmp $(top_srcdir)/graphs/directed/$$i; done

vrml:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tvrml -o$$i.vrml $(top_srcdir)/graphs/directed/$$i; done

vtx:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Tvtx -o$$i.vtx $(top_srcdir)/graphs/directed/$$i; done

xdot:
	for i in $(GRAPHS); do echo "dot $$i"; \
		$(DOT) -Txdot -o$$i.xdot $(top_srcdir)/graphs/directed/$$i; done
