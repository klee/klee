:PACKAGE: graphviz

.SOURCE.h : ../../lib/common
/*
.SOURCE.h : ../../lib/common ../../lib/gvc ../../lib/pathplan ../../lib/graph \
	../../lib/cdt
*/

/*
.SOURCE.a : ../../lib/cdt ../../lib/graph ../../lib/common ../../lib/gvc \
    ../../lib/pathplan 

.SOURCE.a : ../../lib/cdt ../../lib/graph ../../lib/common ../../lib/gvc \
    ../../lib/gd ../../lib/pathplan ../../lib/pack ../../lib/dotgen \
    ../../lib/neatogen ../../lib/fdpgen ../../lib/twopigen ../../lib/circogen \
    ../../plugin/gd ../../plugin/core ../../plugin/dot_layout \
    ../../plugin/neato_layout ../../plugin/pango
*/

dot :: dot.1 dot.c -lgvc -ldotgen -ltwopigen -lcircogen -lfdpgen \
	-lgvplugin_core -lgvplugin_dot_layout -lgvplugin_neato_layout \
	-lgvplugin_gd -lgvplugin_pango -lcommon -lpathplan

:: dot.pdf

