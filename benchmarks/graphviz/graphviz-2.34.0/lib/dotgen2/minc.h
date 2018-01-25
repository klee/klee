/* $Id$Revision: */
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


#ifndef         MINC_H
#define         MINC_H

typedef enum {mcGrVer,mcGrHor,mcBoth} mcGroupAlign_t;
typedef enum {mcGrLoose,mcGrStrict} mcGroup_t;
typedef enum {mcCountLower,mcCountHigher,mcCountAll} mcCountMethod;

typedef struct mcNode mcNode;
typedef struct {
    mcGroupAlign_t alignType;
    mcGroup_t type;
    int cnt;
    Dt_t* nodes;
#if 0
    Dt_t* children;
#endif
} mcGroup;

typedef struct {
    Agedge_t* e; /*pointer to cgraph edge*/
    mcNode* lowerN;
    mcNode* higherN;
    port lowerPort;
    port higherPort;
    float penalty;
} mcEdge;

struct mcNode {
    float order;
    float groupOrder;
    int rank;
    int isLeader;
    mcNode* leader;
    float prevOrder[3];
    float x;
    float y;
    float w;    /*weight*/
    mcEdge** low_edgs;
    mcEdge** high_edgs;
    int high_edgs_cnt;
    int low_edgs_cnt;
    Agnode_t* node;
    mcGroup* group;
    void* level;
    int flag; /*general purpose flag*/
    char* name;
};

typedef struct {
    int rank;
    mcNode** nodes;
    int nodecnt;
    int lowerEdgeCount;
    int higherEdgeCount;
    int lowCnt;   /*cached crossing counts*/
    int highCnt;   /*cached crossing counts*/
    int candidate;
} mcLevel;

typedef struct {
    mcLevel** lvls;
    int lvl_cnt;
    int crossCnt;
    Agraph_t* g;
} mcGraph;

typedef struct {
    Agrec_t hdr;
    node_t  *set;                           /* for union-find */
    graph_t *cluster;                       /* lowest containing cluster */
    union {
	node_t  *v;
	edge_t  *e;
	graph_t *g;
    } rep;
    Agnode_t* orig;
    int     type;
    int     rank;
    int     order;
    float orderr;
    mcNode* mcn;
    graph_t *highCluster;                   /* highest containing cluster */
    int inBucket;
} mcnodeinfo_t;

#define MND_FIELD(g,field)  (((mcnodeinfo_t*)((g)->base.data))->field)
#define MND_orig(n)	MND_FIELD(n,orig)
#define MND_type(n)	MND_FIELD(n,type)
#define MND_rank(n)	MND_FIELD(n,rank)
#define MND_order(n)	MND_FIELD(n,order)
#define MND_orderr(n)	MND_FIELD(n,orderr)
#define MND_mcn(n)	MND_FIELD(n,mcn)
#define MND_inBucket(n)	MND_FIELD(n,inBucket)
#define MND_highCluster(n)	MND_FIELD(n,highCluster)

typedef struct {
    Agrec_t hdr;
    Agedge_t* orig;
    port  tail_port, head_port;
    int deleteFlag;
    int cutval;
    int type;
} mcedgeinfo_t;

#define MED_FIELD(g,field)  (((mcedgeinfo_t*)((g)->base.data))->field)
#define MED_type(n)	MED_FIELD(n,type)
#define MED_orig(n)	MED_FIELD(n,orig)
#define MED_deleteFlag(n)	MED_FIELD(n,deleteFlag)
#define MED_head_port(n)	MED_FIELD(n,head_port)
#define MED_tail_port(n)	MED_FIELD(n,tail_port)

extern Agraph_t* mkMCGraph (Agraph_t*);
extern Agnode_t *mkMCNode (Agraph_t * g, int type, char *name);
extern Agedge_t* mkMCEdge (Agraph_t* g, Agnode_t* tail, Agnode_t* head, char*, int type, Agedge_t* orige);

#endif
