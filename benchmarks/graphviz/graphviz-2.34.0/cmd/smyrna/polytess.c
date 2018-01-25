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

#include "polytess.h"
#include "xdot.h"
tessPoly TP;
#if 0
GLdouble star[5][3] = { 0.6f,  -0.1f, 0.0f,
                        1.35f, 1.4f, 0.0f,
                        2.1f,  -0.1f, 0.0f,
                        0.6f, 0.9f, 0.0f,
                        2.1f, 0.9f, 0.0f,};


//second polygon: a quad-4 vertices; first contour

GLdouble s1[3][3] = { 0.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,};
GLdouble s2[4][3] = {	0.0f, 1.0f, 0.0f,
			0.3f, 0.3f, 0.0f,
			0.7f, 0.3f, 0.0f, 
			0.5f, 0.7f, 0.0f,};






GLdouble quad2[9][3] = { 0.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f,
			0.3f, 0.3f, 0.0f,
                       0.7f, 0.3f, 0.0f, 
                       0.5f, 0.7f, 0.0f,
			0.3f, 0.3f, 0.0f};



GLdouble quad[5][3] = { 0.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f};

//second polygon: a triangle-3 vertices; second contour
GLdouble tri[3][3] = { 0.3f, 0.3f, 0.0f,
                       0.7f, 0.3f, 0.0f,
                       0.5f, 0.7f, 0.0f};

GLdouble complex[25][3] = { 0.0f, 0.0f, 0.0f,
                        2.0f, 1.0f, 0.0f,
                        2.0f, 2.0f, 0.0f,
                        1.0f, 3.0f, 0.0f,
			3.0f, 3.0f, 0.0f,
			3.0f, 5.0f, 0.0f,
                       0.0f, 5.0f, 0.0f, 
                       1.0f, 6.0f, 0.0f,
			0.0f, 7.0f, 0.0f,
			3.0f, 7.0f, 0.0f,
			5.0f, 5.0f, 0.0f,
			6.0f, 7.0f, 0.0f,
			7.0f, 7.0f, 0.0f,
			6.0f, 4.0f, 0.0f,
			5.0f, 4.0f, 0.0f,
			5.0f, 3.0f, 0.0f,
			4.0f, 3.0f, 0.0f,
			4.0f, 2.0f, 0.0f,
			7.0f, 2.0f, 0.0f,
			4.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f,
			2.5f, 0.5f, 0.0f,
			3.5f, 1.0f, 0.0f,
			3.5f, 0.5f, 0.0f,
			2.5f, 0.5f, 0.0f};



#endif
#ifndef WIN32
#define CALLBACK 
#endif
static void CALLBACK combineCallback(GLdouble coords[3], GLdouble *vertex_data[4],GLfloat weight[4], GLdouble **dataOut)
{
    GLdouble *vertex;
    int i;
    vertex = (GLdouble *) malloc(6 * sizeof(GLdouble));
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    vertex[2] = coords[2];
    for (i = 3; i < 6; i++)
    {
/*	vertex[i] = weight[0] * vertex_data[0][i] +
		    weight[1] * vertex_data[1][i] +
		    weight[2] * vertex_data[2][i] +
		    weight[3] * vertex_data[3][i];*/
	vertex[i] = 0;

    }
    *dataOut = vertex;
}

static void CALLBACK vertexCallback(GLvoid *vertex)
{
    GLdouble *ptr;
    ptr = (GLdouble *) vertex;
    glVertex3dv((GLdouble *) ptr);

}

static GLUtesselator* Init()
{
    // Create a new tessellation object 
    GLUtesselator* tobj = gluNewTess(); 
    // Set callback functions
    gluTessCallback(tobj, GLU_TESS_VERTEX, &vertexCallback);
    gluTessCallback(tobj, GLU_TESS_BEGIN, &glBegin);
    gluTessCallback(tobj, GLU_TESS_END, &glEnd);
    gluTessCallback(tobj, GLU_TESS_COMBINE,&combineCallback);
    return tobj;
}



static int Set_Winding_Rule(GLUtesselator *tobj,GLenum winding_rule)
{

// Set the winding rule
    gluTessProperty(tobj, GLU_TESS_WINDING_RULE, winding_rule); 
    return 1 ;
}

static int Render_Contour2(GLUtesselator *tobj,sdot_op* p)
{
    GLdouble** d;
    int x=0;
    /* int y=0; */

    d=(GLdouble**) malloc(sizeof(GLdouble)* p->op.u.polygon.cnt);
    for (x=0;x < p->op.u.polygon.cnt; x++)
    {
	/* GLdouble temp; */
	d[x]=(GLdouble*)(malloc(sizeof(GLdouble)*3));
	d[x][0]=p->op.u.polygon.pts[x].x;
	d[x][1]=p->op.u.polygon.pts[x].y;
	d[x][2]=p->op.u.polygon.pts[x].z+view->Topview->global_z;
    }
    for (x = 0; x < p->op.u.polygon.cnt; x++) //loop through the vertices
    {
	gluTessVertex(tobj, d[x],d[x]); //store the vertex
    }


/*    for (x = 0; x < p->op.u.polygon.cnt; x++) //loop through the vertices
    {
	d[0]=p->op.u.polygon.pts[x].x;
	d[1]=p->op.u.polygon.pts[x].y;
	d[2]=p->op.u.polygon.pts[x].z;
//	gluTessVertex(tobj, obj_data[x], obj_data[x]); //store the vertex
	gluTessVertex(tobj, d,d); //store the vertex
    }*/
    return(1);

}

#if UNUSED
static int Render_Contour(GLUtesselator *tobj, GLdouble obj_data[][3],int cnt)
{
//    GLdouble d[1][3];
    static GLdouble** d;
    int x=0;
    /* int y=0; */
    if (!d) {
	d = N_NEW(cnt,GLdouble*);
	for (x=0;x < cnt; x++)
	{
	    /* GLdouble temp; */
	    d[x] = N_NEW3,GLdouble);
	    d[x][0]=obj_data[x][0];
	    d[x][1]=obj_data[x][1];
	    d[x][2]=obj_data[x][2];
        }
    }




    for (x = 0; x < cnt; x++) //loop through the vertices
    {
/*	d[0][0]=obj_data[x][0];
	d[0][1]=obj_data[x][1];
	d[0][2]=obj_data[x][2];*/



/*void APIENTRY  gluTessVertex(       
    GLUtesselator       *tess,
    GLdouble            coords[3], 
    void                *data );*/

	gluTessVertex(tobj, d[x], d[x]); //store the vertex
//	gluTessVertex(tobj, obj_data[x], obj_data[x]); //store the vertex
//	gluTessVertex(tobj, d[0],d[0]); //store the vertex
//	gluTessVertex(tobj, g,g); //store the vertex

    }

    return(1);

}
#endif






static int Begin_Polygon(GLUtesselator *tobj)
{
    gluTessBeginPolygon(tobj, NULL);
    return(1);
}
static int End_Polygon(GLUtesselator *tobj)
{
    gluTessEndPolygon(tobj);
    return(1);

}
static int Begin_Contour(GLUtesselator *tobj)
{
    gluTessBeginContour(tobj);
    return(1);

}
static int End_Contour(GLUtesselator *tobj)
{
    gluTessEndContour(tobj);
    return(1);

}

#if 0
static int freeTes(GLUtesselator *tobj)
{
    gluDeleteTess(tobj);
    return(1);

}
#endif

int drawTessPolygon(sdot_op* p)
{
    if (!TP.tobj)
    {
	TP.tobj=Init();
	TP.windingRule=GLU_TESS_WINDING_ODD;
    }
    Set_Winding_Rule(TP.tobj,TP.windingRule);
    Begin_Polygon(TP.tobj); 
    Begin_Contour(TP.tobj);
    Render_Contour2(TP.tobj,p); 
//    Render_Contour(TP.tobj,complex,25); 
    End_Contour(TP.tobj);
    End_Polygon(TP.tobj);
    return 1;
}

