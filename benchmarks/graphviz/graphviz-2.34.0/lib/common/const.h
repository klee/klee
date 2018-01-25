/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#ifndef GV_CONST_H
#define GV_CONST_H

#define SMALLBUF	128
#define LPAREN		'('
#define RPAREN		')'
#define LBRACE		'{'
#define RBRACE		'}'

/*	node,edge types */
#ifdef NORMAL
#undef NORMAL
#endif
#define		NORMAL		0	/* an original input node */
#define		VIRTUAL		1	/* virtual nodes in long edge chains */
#define		SLACKNODE	2	/* encode edges in node position phase */
#define		REVERSED	3	/* reverse of an original edge */
#define		FLATORDER	4	/* for ordered edges */
#define		CLUSTER_EDGE 5	/* for ranking clusters */
#define		IGNORED		6	/* concentrated multi-edges */

/* collapsed node classifications */
#define		NOCMD		0	/* default */
#define		SAMERANK	1	/* place on same rank */
#define		MINRANK		2	/* place on "least" rank */
#define		SOURCERANK	3	/* strict version of MINRANK */
#define		MAXRANK		4	/* place on "greatest" rank */
#define		SINKRANK	5	/* strict version of MAXRANK */
#define		LEAFSET		6	/* set of collapsed leaf nodes */
#define		CLUSTER		7	/* set of clustered nodes */

/* type of cluster rank assignment */
#define		LOCAL		100
#define		GLOBAL		101
#define		NOCLUST		102

/* default attributes */
#define		DEFAULT_COLOR		"black"
#define		DEFAULT_ACTIVEPENCOLOR	"#808080"
#define		DEFAULT_ACTIVEFILLCOLOR	"#fcfcfc"
#define		DEFAULT_ACTIVEFONTCOLOR	"black"
#define		DEFAULT_SELECTEDPENCOLOR "#303030"
#define		DEFAULT_SELECTEDFILLCOLOR "#e8e8e8"
#define		DEFAULT_SELECTEDFONTCOLOR "black"
#define		DEFAULT_DELETEDPENCOLOR	"#e0e0e0"
#define		DEFAULT_DELETEDFILLCOLOR "#f0f0f0"
#define		DEFAULT_DELETEDFONTCOLOR "darkgrey"
#define		DEFAULT_VISITEDPENCOLOR	"#101010"
#define		DEFAULT_VISITEDFILLCOLOR "#f8f8f8"
#define		DEFAULT_VISITEDFONTCOLOR "black"
#define		DEFAULT_FONTSIZE	14.0
#define		DEFAULT_LABEL_FONTSIZE	11.0	/* for head/taillabel */
#define		MIN_FONTSIZE		1.0
#define		DEFAULT_FONTNAME	"Times-Roman"
#define		DEFAULT_FILL		"lightgrey"
#define         LINESPACING             1.20

#define		DEFAULT_NODEHEIGHT	0.5
#define		MIN_NODEHEIGHT		0.02
#define		DEFAULT_NODEWIDTH	0.75
#define		MIN_NODEWIDTH		0.01
#define		DEFAULT_NODESHAPE	"ellipse"

#define		NODENAME_ESC		"\\N"

#define		DEFAULT_LAYERSEP	":\t "
#define		DEFAULT_LAYERLISTSEP	","

#define		DEFAULT_NODESEP	0.25
#define		MIN_NODESEP		0.02
#define		DEFAULT_RANKSEP	0.5
#define		MIN_RANKSEP		0.02


/* default margin for paged formats such as PostScript - in points  = 0.5in */
#define		DEFAULT_PRINT_MARGIN 36
/* default margin for embedded formats such as PNG - in points */
#define		DEFAULT_EMBED_MARGIN 0
/* default padding around graph - in points */
#define		DEFAULT_GRAPH_PAD 4

#define		SELF_EDGE_SIZE	18
#define		MC_SCALE	256	/* for mincross */

#define		PORT_LABEL_DISTANCE	10
#define		PORT_LABEL_ANGLE	-25	/* degrees; pos is CCW, neg is CW */

/* default polygon sample size for overlap removal and maps */
#define DFLT_SAMPLE 20

/* arrow types */
#define		ARR_NONE         0

/* sides (e.g. of cluster margins) */
#define		BOTTOM_IX	0
#define		RIGHT_IX	1
#define		TOP_IX		2
#define		LEFT_IX		3

/* sides of boxes for SHAPE_path */
#define		BOTTOM		(1<<BOTTOM_IX)
#define		RIGHT		(1<<RIGHT_IX)
#define		TOP		(1<<TOP_IX)
#define		LEFT		(1<<LEFT_IX)

#define CCW  -1			/* counter clock-wise */
#define CW    1			/* clock-wise */

/* Obj types  for renderers */
#define		NONE    0
#define		NODE    1
#define		EDGE    2
#define		CLST    3

/* output languages */
#define		HPGL		2	/* HP Graphics Language */
#define		PCL		3	/* Printer Control Language */
#define		MIF		4	/* Adobe FrameMaker */
#define		PIC_format	5	/* symbol PIC is used by compilers for 
					   Position Independent Code */
#define		TK		15	/* TK canvas */

#define		VTX		21	/* visual thought */
#define		METAPOST	22
#define		DIA		24	/* dia drawing tool */

#define		QPDF		30	/* Quartz paged PDF */
#define		QEPDF		31	/* Quartz embedded PDF */

#define		QBM_FIRST 100	/* first Quartz bitmap format, inclusive */
#define		QBM_LAST 200	/* last Quartz bitmap format, exclusive */

#define		GVRENDER_PLUGIN	300	/* a plugin supported language */
#define		NO_SUPPORT	999	/* no support */

/* for clusters */
#define		CL_BACK		10	/* cost of backward pointing edge */
#define		CL_OFFSET	8	/* margin of cluster box in PS points */
#ifndef MSWIN32
#define		CL_CROSS	1000	/* cost of cluster skeleton edge crossing */
#else
#define		CL_CROSS	100	/* avoid 16 bit overflow */
#endif

/* edge types */
#define REGULAREDGE 1
#define FLATEDGE    2
#define SELFWPEDGE  4
#define SELFNPEDGE  8
#define SELFEDGE    8
#define EDGETYPEMASK	 15	/* the OR of the above */

/* for graph server */
#define		SERVER_NN	200
#define		SERVER_NE	500

/* for neato */
#define Spring_coeff    1.0
#define MYHUGE          (1.0e+37)
#define MAXDIM			10

/* drawing phases */
#define GVBEGIN         0
#define GVSPLINES       1

/* existence of labels */
#define EDGE_LABEL		(1 << 0)
#define HEAD_LABEL		(1 << 1)
#define TAIL_LABEL		(1 << 2)
#define GRAPH_LABEL		(1 << 3)
#define NODE_XLABEL		(1 << 4)
#define EDGE_XLABEL		(1 << 5)

/* type of graph label: GD_label_pos */
#define LABEL_AT_BOTTOM	0
#define LABEL_AT_TOP	1
#define LABEL_AT_LEFT	2
#define LABEL_AT_RIGHT	4

/* values specifying rankdir */
#define RANKDIR_TB	0
#define RANKDIR_LR	1
#define RANKDIR_BT	2
#define RANKDIR_RL	3

/* allowed charsets */
#define CHAR_UTF8	0
#define CHAR_LATIN1	1
#define CHAR_BIG5	2

/* style flags (0-23)*/
#define FILLED		(1 << 0)
#define RADIAL		(1 << 1)
#define ROUNDED		(1 << 2)
#define DIAGONALS	(1 << 3)
#define AUXLABELS	(1 << 4)
#define INVISIBLE	(1 << 5)
#define STRIPED		(1 << 6)
#define DOTTED		(1 << 7)
#define DASHED		(1 << 8)
#define WEDGED		(1 << 9)

#define SHAPE_MASK	(127 << 24)

/* shape (1-127) */
#define DOGEAR		(1 << 24)
#define TAB		(2 << 24)
#define FOLDER		(3 << 24)
#define BOX3D		(4 << 24)
#define COMPONENT	(5 << 24)
#define PROMOTER	(6 << 24)
#define CDS		(7 << 24)
#define TERMINATOR	(8 << 24)
#define UTR		(9 << 24)
#define PRIMERSITE	(10 << 24)
#define RESTRICTIONSITE	(11 << 24)
#define FIVEPOVERHANG	(12 << 24)
#define THREEPOVERHANG	(13 << 24)
#define NOVERHANG	(14 << 24)
#define ASSEMBLY	(15 << 24)
#define SIGNATURE	(16 << 24)
#define INSULATOR	(17 << 24)
#define RIBOSITE	(18 << 24)
#define RNASTAB		(19 << 24)
#define PROTEASESITE	(20 << 24)
#define PROTEINSTAB	(21 << 24)
#define RPROMOTER	(22 << 24)
#define RARROW		(23 << 24)
#define LARROW		(24 << 24)
#define LPROMOTER	(25 << 24)

/* fill types */
#define FILL      	1
#define GRADIENT  	2
#define RGRADIENT 	3
#define NO_POLY   	4    /* bit flag: if set, do fill only */

/* label types */
#define LT_NONE 	(0 << 1)
#define LT_HTML 	(1 << 1)
#define LT_RECD 	(2 << 1)

/* Flags stored in GD_flags 
 * Bit(s):  0     HAS_CLUST_EDGE
 *          1-3   ET_ 
 *          4     NEW_RANK
 */

/* edge types */
#define ET_NONE 	(0 << 1)
#define ET_LINE 	(1 << 1)
#define ET_CURVED 	(2 << 1)
#define ET_PLINE 	(3 << 1)
#define ET_ORTHO 	(4 << 1)
#define ET_SPLINE 	(5 << 1)
#define ET_COMPOUND 	(6 << 1)

/* New ranking is used */
#define NEW_RANK    	(1 << 4)
/******/

/* user-specified node position: ND_pinned */
#define P_SET    1		/* position supplied by user */
#define P_FIX    2		/* position fixed during topological layout */
#define P_PIN    3		/* position fixed */

#define GAP 4			/* whitespace in POINTS around labels and between peripheries */

/* fontsize at which text is omitted entirely */
#define FONTSIZE_MUCH_TOO_SMALL 0.15
/* fontsize at which text is rendered by a simple line */
#define FONTSIZE_TOO_SMALL 1.5

#endif
