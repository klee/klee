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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "const.h"

#include "gvplugin_render.h"
#include "gvplugin_device.h"
#include "gvio.h"
#include "memory.h"

typedef enum { FORMAT_VML, FORMAT_VMLZ, } format_type;

unsigned int  graphHeight,graphWidth;

#ifndef HAVE_STRCASECMP
extern int strcasecmp(const char *s1, const char *s2);
#endif

/*  this is a direct copy fromlib/common/labels.c  */
static int xml_isentity(char *s)
{
    s++;			/* already known to be '&' */
    if (*s == '#') {
	s++;
	if (*s == 'x' || *s == 'X') {
	    s++;
	    while ((*s >= '0' && *s <= '9')
		   || (*s >= 'a' && *s <= 'f')
		   || (*s >= 'A' && *s <= 'F'))
		s++;
	} else {
	    while (*s >= '0' && *s <= '9')
		s++;
	}
    } else {
	while ((*s >= 'a' && *s <= 'z')
	       || (*s >= 'A' && *s <= 'Z'))
	    s++;
    }
    if (*s == ';')
	return 1;
    return 0;
}

static void vml_bzptarray(GVJ_t * job, pointf * A, int n)
{
    int i;
    char *c;

    c = "m ";			/* first point */
    for (i = 0; i < n; i++) {
        /* integers only in path! */
	gvprintf(job, "%s%.0f,%.0f ", c, A[i].x, graphHeight-A[i].y);
	if (i == 0)
	    c = "c ";		/* second point */
	else
	    c = "";		/* remaining points */
    }
    gvputs(job, "\"");
}

static void vml_print_color(GVJ_t * job, gvcolor_t color)
{
    switch (color.type) {
    case COLOR_STRING:
	gvputs(job, color.u.string);
	break;
    case RGBA_BYTE:
	if (color.u.rgba[3] == 0) /* transparent */
	    gvputs(job, "none");
	else
	    gvprintf(job, "#%02x%02x%02x",
		color.u.rgba[0], color.u.rgba[1], color.u.rgba[2]);
	break;
    default:
	assert(0);		/* internal error */
    }
}

static void vml_grstroke(GVJ_t * job, int filled)
{
    obj_state_t *obj = job->obj;

    gvputs(job, "<v:stroke color=\"");
    vml_print_color(job, obj->pencolor);
    if (obj->penwidth != PENWIDTH_NORMAL)
	gvprintf(job, "\" weight=\"%.0fpt", obj->penwidth);
    if (obj->pen == PEN_DASHED) {
	gvputs(job, "\" dashstyle=\"dash");
    } else if (obj->pen == PEN_DOTTED) {
	gvputs(job, "\" dashstyle=\"dot");
    }
    gvputs(job, "\" />");
}


static void vml_grfill(GVJ_t * job, int filled)
{
    obj_state_t *obj = job->obj;

    if (filled){
        gvputs(job, " filled=\"true\" fillcolor=\"");
	vml_print_color(job, obj->fillcolor);
        gvputs(job, "\" ");
    }else{
	gvputs(job, " filled=\"false\" ");
    }
}

/*  html_string is a modified version of xml_string  */
char *html_string(char *s)
{
    static char *buf = NULL;
    static int bufsize = 0;
    char *p, *sub, *prev = NULL;
    int len, pos = 0;
    int temp,cnt,remaining=0;
    char workstr[16];
    long unsigned int charnum=0;
    unsigned char byte;
    unsigned char mask;


    if (!buf) {
	bufsize = 64;
	buf = gmalloc(bufsize);
    }
    p = buf;
    while (s && *s) {
	if (pos > (bufsize - 8)) {
	    bufsize *= 2;
	    buf = grealloc(buf, bufsize);
	    p = buf + pos;
	}
	/* escape '&' only if not part of a legal entity sequence */
	if (*s == '&' && !(xml_isentity(s))) {
	    sub = "&amp;";
	    len = 5;
	}
	/* '<' '>' are safe to substitute even if string is already UTF-8 coded
	 * since UTF-8 strings won't contain '<' or '>' */
	else if (*s == '<') {
	    sub = "&lt;";
	    len = 4;
	}
	else if (*s == '>') {
	    sub = "&gt;";
	    len = 4;
	}
	else if (*s == '-') {	/* can't be used in xml comment strings */
	    sub = "&#45;";
	    len = 5;
	}
	else if (*s == ' ' && prev && *prev == ' ') {
	    /* substitute 2nd and subsequent spaces with required_spaces */
	    sub = "&#160;";  /* inkscape doesn't recognise &nbsp; */
	    len = 6;
	}
	else if (*s == '"') {
	    sub = "&quot;";
	    len = 6;
	}
	else if (*s == '\'') {
	    sub = "&#39;";
	    len = 5;
	}
	else if ((unsigned char)*s > 127) {
            byte=(unsigned char)*s;
            cnt=0;
            for (mask=127; mask < byte; mask=mask >>1){
              cnt++;
              byte=byte & mask;
            }
            if (cnt>1){
              charnum=byte;
              remaining=cnt-1;
            }else{
              charnum=charnum<<6;
              charnum+=byte;
              remaining--;
            }
            if (remaining>0){
              s++;
              continue;
            }           
            /* we will build the html value right-to-left
             * (least significant-to-most)  */
            workstr[15]=';';
            sub=&workstr[14];
            len=3; /*  &#  + ;  */
            do {
              temp=charnum%10;
              *(sub--)=(char)((int)'0'+ temp);
              charnum/=10;
              len++;
              if (len>12){      /* 12 is arbitrary, but clearly in error  */
                fprintf(stderr, "Error during conversion to \"UTF-8\".  Quiting.\n");
                exit(1);
              }
            } while (charnum>0);
            *(sub--)='#';
            *(sub)='&';
	}
	else {
	    sub = s;
	    len = 1;
	}
	while (len--) {
	    *p++ = *sub++;
	    pos++;
	}
	prev = s;
	s++;
    }
    *p = '\0';
    return buf;
}
static void vml_comment(GVJ_t * job, char *str)
{
    gvputs(job, "      <!-- ");
    gvputs(job, html_string(str));
    gvputs(job, " -->\n");
}
static void vml_begin_job(GVJ_t * job)
{
    gvputs(job, "<HTML>\n");
    gvputs(job, "\n<!-- Generated by ");
    gvputs(job, html_string(job->common->info[0]));
    gvputs(job, " version ");
    gvputs(job, html_string(job->common->info[1]));
    gvputs(job, " (");
    gvputs(job, html_string(job->common->info[2]));
    gvputs(job, ")\n-->\n");
}

static void vml_begin_graph(GVJ_t * job)
{
    obj_state_t *obj = job->obj;
    char *name;

    graphHeight =(int)(job->bb.UR.y - job->bb.LL.y);
    graphWidth  =(int)(job->bb.UR.x - job->bb.LL.x);

    gvputs(job, "<HEAD>");
    gvputs(job, "<META http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n");
 

#ifndef WITH_CGRAPH
    name = obj->u.g->name;
#else
    name = agnameof(obj->u.g);
#endif
    if (name[0]) {
        gvputs(job, "<TITLE>");
	gvputs(job, html_string(name));
        gvputs(job, "</TITLE>");
    }
    gvprintf(job, "<!-- Pages: %d -->\n", job->pagesArraySize.x * job->pagesArraySize.y);

/*  the next chunk and all the "DIV" stuff is not required,
 *  but it helps with non-IE browsers  */
    gvputs(job, "   <SCRIPT LANGUAGE='Javascript'>\n");
    gvputs(job, "   function browsercheck()\n");
    gvputs(job, "   {\n");
    gvputs(job, "      var ua = window.navigator.userAgent\n");
    gvputs(job, "      var msie = ua.indexOf ( 'MSIE ' )\n");
    gvputs(job, "      var ievers;\n");
    gvputs(job, "      var item;\n");
    gvputs(job, "      var VMLyes=new Array('_VML1_','_VML2_');\n");
    gvputs(job, "      var VMLno=new Array('_notVML1_','_notVML2_');\n");
    gvputs(job, "      if ( msie > 0 ){      // If Internet Explorer, return version number\n");
    gvputs(job, "         ievers= parseInt (ua.substring (msie+5, ua.indexOf ('.', msie )))\n");
    gvputs(job, "      }\n");
    gvputs(job, "      if (ievers>=5){\n");
    gvputs(job, "       for (x in VMLyes){\n");
    gvputs(job, "         item = document.getElementById(VMLyes[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='visible';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "       for (x in VMLno){\n");
    gvputs(job, "         item = document.getElementById(VMLno[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='hidden';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "     }else{\n");
    gvputs(job, "       for (x in VMLyes){\n");
    gvputs(job, "         item = document.getElementById(VMLyes[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='hidden';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "       for (x in VMLno){\n");
    gvputs(job, "         item = document.getElementById(VMLno[x]);\n");
    gvputs(job, "         if (item) {\n");
    gvputs(job, "           item.style.visibility='visible';\n");
    gvputs(job, "         }\n");
    gvputs(job, "       }\n");
    gvputs(job, "     }\n");
    gvputs(job, "   }\n");
    gvputs(job, "   </SCRIPT>\n");

    gvputs(job, "</HEAD>");
    gvputs(job, "<BODY onload='browsercheck();'>\n");
    /* add 10pt pad to the bottom of the graph */
    gvputs(job, "<DIV id='_VML1_' style=\"position:relative; display:inline; visibility:hidden");
    gvprintf(job, " width: %dpt; height: %dpt\">\n", graphWidth, 10+graphHeight);
    gvputs(job, "<STYLE>\n");
    gvputs(job, "v\\:* { behavior: url(#default#VML);display:inline-block}\n"); 
    gvputs(job, "</STYLE>\n"); 
    gvputs(job, "<xml:namespace ns=\"urn:schemas-microsoft-com:vml\" prefix=\"v\" />\n"); 

    gvputs(job, " <v:group style=\"position:relative; ");
    gvprintf(job, " width: %dpt; height: %dpt\"", graphWidth, graphHeight);
    gvprintf(job, " coordorigin=\"0,0\" coordsize=\"%d,%d\" >", graphWidth, graphHeight);
}

static void vml_end_graph(GVJ_t * job)
{
   gvputs(job, "</v:group>\n");
   gvputs(job, "</DIV>\n");
   /* add 10pt pad to the bottom of the graph */
   gvputs(job, "<DIV id='_VML2_' style=\"position:relative;visibility:hidden\">\n");
   gvputs(job, "<!-- insert any other html content here -->\n");
   gvputs(job, "</DIV>\n");
   gvputs(job, "<DIV id='_notVML1_' style=\"position:relative;\">\n");
   gvputs(job, "<!-- this should only display on NON-IE browsers -->\n");
   gvputs(job, "<H2>Sorry, this diagram will only display correctly on Internet Explorer 5 (and up) browsers.</H2>\n");
   gvputs(job, "</DIV>\n");
   gvputs(job, "<DIV id='_notVML2_' style=\"position:relative;\">\n");
   gvputs(job, "<!-- insert any other NON-IE html content here -->\n");
   gvputs(job, "</DIV>\n");

   gvputs(job, "</BODY>\n</HTML>\n");
}

static void
vml_begin_anchor(GVJ_t * job, char *href, char *tooltip, char *target, char *id)
{
    gvputs(job, "<a");
    if (href && href[0])
	gvprintf(job, " href=\"%s\"", html_string(href));
    if (tooltip && tooltip[0])
	gvprintf(job, " title=\"%s\"", html_string(tooltip));
    if (target && target[0])
	gvprintf(job, " target=\"%s\"", html_string(target));
    gvputs(job, ">\n");
}

static void vml_end_anchor(GVJ_t * job)
{
    gvputs(job, "</a>\n");
}

static void vml_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
    pointf p1,p2;
    obj_state_t *obj = job->obj;

    switch (para->just) {
    case 'l':
	p1.x=p.x;
	break;
    case 'r':
	p1.x=p.x-para->width;
	break;
    default:
    case 'n':
	p1.x=p.x-(para->width/2);
	break;
    }
    p2.x=p1.x+para->width;
    if (para->height <  para->fontsize){
      para->height = 1 + (1.1*para->fontsize);
    }

    p1.x-=8; /* vml textbox margin fudge factor */
    p2.x+=8; /* vml textbox margin fudge factor */
    p2.y=graphHeight-(p.y);
    p1.y=(p2.y-para->height);
    /* text "y" was too high
     * Graphviz uses "baseline", VML seems to use bottom of descenders - so we fudge a little
     * (heuristics - based on eyeballs)  */
    if (para->fontsize <12.){ /*     see graphs/directed/arrows.gv  */
      p1.y+=1.4+para->fontsize/5; /* adjust by approx. descender */
      p2.y+=1.4+para->fontsize/5; /* adjust by approx. descender */
    }else{
      p1.y+=2+para->fontsize/5; /* adjust by approx. descender */
      p2.y+=2+para->fontsize/5; /* adjust by approx. descender */
    }

    gvprintf(job, "<v:rect style=\"position:absolute; ");
    gvprintf(job, " left: %.2f; top: %.2f;", p1.x, p1.y);
    gvprintf(job, " width: %.2f; height: %.2f\"", p2.x-p1.x, p2.y-p1.y);
    gvputs(job, " stroked=\"false\" filled=\"false\">\n");
    gvputs(job, "<v:textbox inset=\"0,0,0,0\" style=\"position:absolute; v-text-wrapping:'false';padding:'0';");

    if (para->postscript_alias) {
        gvprintf(job, "font-family: '%s';", para->postscript_alias->family);
        if (para->postscript_alias->weight)
	    gvprintf(job, "font-weight: %s;", para->postscript_alias->weight);
        if (para->postscript_alias->stretch)
	    gvprintf(job, "font-stretch: %s;", para->postscript_alias->stretch);
        if (para->postscript_alias->style)
	    gvprintf(job, "font-style: %s;", para->postscript_alias->style);
    }
    else {
        gvprintf(job, "font-family: \'%s\';", para->fontname);
    }
    gvprintf(job, " font-size: %.2fpt;", para->fontsize);
    switch (obj->pencolor.type) {
    case COLOR_STRING:
	if (strcasecmp(obj->pencolor.u.string, "black"))
	    gvprintf(job, "color:%s;", obj->pencolor.u.string);
	break;
    case RGBA_BYTE:
	gvprintf(job, "color:#%02x%02x%02x;",
		obj->pencolor.u.rgba[0], obj->pencolor.u.rgba[1], obj->pencolor.u.rgba[2]);
	break;
    default:
	assert(0);		/* internal error */
    }
    gvputs(job, "\"><center>");
    gvputs(job, html_string(para->str));
    gvputs(job, "</center></v:textbox>\n"); 
    gvputs(job, "</v:rect>\n");
}

static void vml_ellipse(GVJ_t * job, pointf * A, int filled)
{   
    double dx, dy, left, right, top, bottom;

    /* A[] contains 2 points: the center and corner. */
    gvputs(job, "  <v:oval style=\"position:absolute;");

    dx=A[1].x-A[0].x;
    dy=A[1].y-A[0].y;

    top=graphHeight-(A[0].y+dy);
    bottom=top+dy+dy;
    left=A[0].x - dx;
    right=A[1].x;
    gvprintf(job, " left: %.2f; top: %.2f;",left, top);
    gvprintf(job, " width: %.2f; height: %.2f\"", 2*dx, 2*dy);

    vml_grfill(job, filled);
    gvputs(job, " >");
    vml_grstroke(job, filled);
    gvputs(job, "</v:oval>\n");
}

static void
vml_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start,
	      int arrow_at_end, int filled)
{
    gvputs(job, " <v:shape style=\"position:absolute; ");
    gvprintf(job, " width: %d; height: %d\"", graphWidth, graphHeight);

    vml_grfill(job, filled);
    gvputs(job, " >");
    vml_grstroke(job, filled);
    gvputs(job, "<v:path  v=\"");
    vml_bzptarray(job, A, n);
    gvputs(job, "/></v:shape>\n");
}

static void vml_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    int i;
    double px,py;

    gvputs(job, " <v:shape style=\"position:absolute; ");
    gvprintf(job, " width: %d; height: %d\"", graphWidth, graphHeight);
    vml_grfill(job, filled);
    gvputs(job, " >");
    vml_grstroke(job, filled);

    gvputs(job, "<v:path  v=\"");
    for (i = 0; i < n; i++)
    {
        px=A[i].x;
        py= (graphHeight-A[i].y);
        if (i==0){
          gvputs(job, "m ");
        }
        /* integers only in path */
	gvprintf(job, "%.0f %.0f ", px, py);
        if (i==0) gvputs(job, "l ");
        if (i==n-1) gvputs(job, "x e \"/>");
    }
    gvputs(job, "</v:shape>\n");
}

static void vml_polyline(GVJ_t * job, pointf * A, int n)
{
    int i;

    gvputs(job, " <v:shape style=\"position:absolute; ");
    gvprintf(job, " width: %d; height: %d\" filled=\"false\">", graphWidth, graphHeight);
    gvputs(job, "<v:path v=\"");
    for (i = 0; i < n; i++)
    {
        if (i==0) gvputs(job, " m ");
	gvprintf(job, "%.0f,%.0f ", A[i].x, graphHeight-A[i].y);
        if (i==0) gvputs(job, " l ");
        if (i==n-1) gvputs(job, " e "); /* no x here for polyline */
    }
    gvputs(job, "\"/>");
    vml_grstroke(job, 0);                 /* no fill here for polyline */
    gvputs(job, "</v:shape>\n");
}

/* color names from 
  http://msdn.microsoft.com/en-us/library/bb250525(VS.85).aspx#t.color
*/
/* NB.  List must be LANG_C sorted */
static char *vml_knowncolors[] = {
     "aqua",  "black",  "blue", "fuchsia", 
     "gray",  "green", "lime",  "maroon", 
     "navy",  "olive",  "purple",  "red",
     "silver",  "teal",  "white",  "yellow"
};

gvrender_engine_t vml_engine = {
    vml_begin_job,
    0,				/* vml_end_job */
    vml_begin_graph,
    vml_end_graph,
    0,                          /* vml_begin_layer */
    0,                          /* vml_end_layer */
    0,                          /* vml_begin_page */
    0,                          /* vml_end_page */
    0,                          /* vml_begin_cluster */
    0,                          /* vml_end_cluster */
    0,				/* vml_begin_nodes */
    0,				/* vml_end_nodes */
    0,				/* vml_begin_edges */
    0,				/* vml_end_edges */
    0,                          /* vml_begin_node */
    0,                          /* vml_end_node */
    0,                          /* vml_begin_edge */
    0,                          /* vml_end_edge */
    vml_begin_anchor,
    vml_end_anchor,
    0,                          /* vml_begin_label */
    0,                          /* vml_end_label */
    vml_textpara,
    0,				/* vml_resolve_color */
    vml_ellipse,
    vml_polygon,
    vml_bezier,
    vml_polyline,
    vml_comment,
    0,				/* vml_library_shape */
};

gvrender_features_t render_features_vml = {
    GVRENDER_Y_GOES_DOWN
        | GVRENDER_DOES_TRANSFORM
	| GVRENDER_DOES_LABELS
	| GVRENDER_DOES_MAPS
	| GVRENDER_DOES_TARGETS
	| GVRENDER_DOES_TOOLTIPS, /* flags */
    0.,                         /* default pad - graph units */
    vml_knowncolors,		/* knowncolors */
    sizeof(vml_knowncolors) / sizeof(char *),	/* sizeof knowncolors */
    RGBA_BYTE,			/* color_type */
};

gvdevice_features_t device_features_vml = {
    GVDEVICE_DOES_TRUECOLOR,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

gvdevice_features_t device_features_vmlz = {
    GVDEVICE_DOES_TRUECOLOR
      | GVDEVICE_COMPRESSED_FORMAT,	/* flags */
    {0.,0.},			/* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},			/* default dpi */
};

gvplugin_installed_t gvrender_vml_types[] = {
    {FORMAT_VML, "vml", 1, &vml_engine, &render_features_vml},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_vml_types[] = {
    {FORMAT_VML, "vml:vml", 1, NULL, &device_features_vml},
#if HAVE_LIBZ
    {FORMAT_VMLZ, "vmlz:vml", 1, NULL, &device_features_vmlz},
#endif
    {0, NULL, 0, NULL, NULL}
};

