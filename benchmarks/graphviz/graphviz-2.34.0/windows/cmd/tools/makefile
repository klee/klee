:PACKAGE: graphviz

.SOURCE.h : ../../lib/ingraphs ../../lib/agraph ../../lib/graph ../../lib/neatogen ../../lib/common ../../lib/gvc
.SOURCE.a : ../../lib/ingraphs

all : acyclic bcomps ccomps dijkstra gc gvcolor gvpack gxl2dot nop sccmap \
	tred unflatten

acyclic :: acyclic.1 acyclic.c -lgraph -lcdt -lingraphs 
bcomps :: bcomps.1 bcomps.c -lgraph -lcdt -lingraphs 
ccomps :: ccomps.1 ccomps.c -lgraph -lcdt -lingraphs 
dijkstra :: dijkstra.1 dijkstra.c -lagraph -lcdt -lcommon -lingraphs 
gc :: gc.1 gc.c -lgraph -lcdt -lingraphs 
gvcolor :: gvcolor.1 gvcolor.c colxlate.c -lgraph -lcdt -lingraphs 
gvpack :: gvpack.1 gvpack.c -ldotgen -ltwopigen -lcircogen -lfdpgen -lgvc \
	-lingraphs -lgvplugin_core -lgvplugin_dot_layout \
	-lgvplugin_neato_layout -lgvplugin_gd -lgvplugin_pango
gxl2dot :: gxl2dot.1 gxl2dot.c dot2gxl.c cvtgxl.c -lagraph \
	-lgraph -lcdt -lcommon -lingraphs
nop :: nop.1 nop.c -lagraph -lcdt -lcommon -lingraphs 
sccmap :: sccmap.1 sccmap.c -lagraph -lcdt -lcommon -lingraphs 
tred :: tred.1 tred.c -lgraph -lcdt -lingraphs 
unflatten :: unflatten.1 unflatten.c -lagraph -lcdt -lcommon -lingraphs 

:: acyclic.pdf bcomps.pdf ccomps.pdf dijkstra.pdf gc.pdf gvcolor.pdf \
	gvpack.pdf gxl2dot.pdf nop.pdf sccmap.pdf tred.pdf unflatten.pdf 

