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

#include "glpangofont.h"

#define DEFAULT_FONT_FAMILY "Arial"
#define DEFAULT_FONT_SIZE 32
#define ANTIALIAS

static int file_exists(const char *filename)
{
    FILE *file;
    if ((file = fopen(filename, "r"))) {
	fclose(file);
	return 1;
    }
    return 0;
}

static PangoLayout *get_pango_layout(char *markup_text,
				     char *fontdescription, int fontsize,
				     double *width, double *height)
{
    PangoFontDescription *desc;
    PangoFontMap *fontmap;
    PangoContext *context;
    PangoLayout *layout;
    int pango_width, pango_height;
    char *text;
    PangoAttrList *attr_list;
    cairo_font_options_t *options;
    fontmap = pango_cairo_font_map_get_default();
    context =
	pango_cairo_font_map_create_context(PANGO_CAIRO_FONT_MAP(fontmap));
    options = cairo_font_options_create();

    cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);

    cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_FULL);
    cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_ON);
    cairo_font_options_set_subpixel_order(options,
					  CAIRO_SUBPIXEL_ORDER_BGR);
//      pango_cairo_context_set_font_options(context, options);

    desc = pango_font_description_from_string(fontdescription);
//      pango_font_description_set_family(desc, "CENTAUR.TTF");
    pango_font_description_set_size(desc, (gint) (fontsize * PANGO_SCALE));

//      pango_font_description_set_style    (desc,PANGO_STYLE_ITALIC);

    if (!pango_parse_markup
	(markup_text, -1, '\0', &attr_list, &text, NULL, NULL))
	return (PangoLayout *) 0;
    layout = pango_layout_new(context);
    pango_layout_set_text(layout, text, -1);
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_attributes(layout, attr_list);
    pango_font_description_free(desc);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

    if (width || height)
	pango_layout_get_size(layout, &pango_width, &pango_height);

    if (width)
	*width = (double) pango_width / PANGO_SCALE;

    if (height)
	*height = (double) pango_height / PANGO_SCALE;

    return layout;
}

static cairo_status_t
writer(void *closure, const unsigned char *data, unsigned int length)
{
    if (length == fwrite(data, 1, length, (FILE *) closure)) {
	return CAIRO_STATUS_SUCCESS;
    }

    return CAIRO_STATUS_WRITE_ERROR;
}

int glCompCreateFontFile(char *fontdescription, int fs, char *fontfile,
		     float gw, float gh)
{

    char buf[] = " ";
    int ncolumns = 16;
    int counter = 0;
    int X = 0;
    int Y = 0;
    cairo_t *cr;
    cairo_surface_t *surface;
    PangoLayout *layout;
    double width, height;
    FILE *output_file;
    int c;
    int return_value = -1;

    if (file_exists(fontfile))	//checking if font file has already been created
	return 0;
    //create the right size canvas for character set
    surface =
	cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				   (int) ((float) ncolumns * gw),
				   (int) (gh * (float) ncolumns));
    cr = cairo_create(surface);
    //draw a rectangle with same size of canvas
    cairo_rectangle(cr, 0, 0, (float) ncolumns * gw,
		    gh * (float) ncolumns);
    //fill rectangle with black
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_fill(cr);
    //set pen color to white
    cairo_set_source_rgb(cr, 1, 1, 1);

    for (c = 0; c < 256; c++) {
	counter++;
	if ((c != 38) && (c != 60) && (c != 128) && (c < 129))
	    buf[0] = c;
	else
	    buf[0] = ' ';
	cairo_move_to(cr, X, Y);
	layout =
	    get_pango_layout(buf, fontdescription, fs, &width, &height);
	pango_cairo_show_layout(cr, layout);
	X = X + (int) gw;
	if (counter == ncolumns) {
	    X = 0;
	    Y = Y + (int) gh;
	    counter = 0;
	}
    }

    output_file = fopen(fontfile, "wb+");
    if (output_file) {
	cairo_surface_write_to_png_stream(surface, writer, output_file);
	return_value = 0;
    }
    fclose(output_file);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return return_value;
}


unsigned char *glCompCreatePangoTexture(char *fontdescription, int fontsize,
				    char *txt, cairo_surface_t * surface,
				    int *w, int *h)
{

//    char buf[] = " ";
//    int ncolumns = 16;
//    int counter = 0;
//    int X = 0;
//    int Y = 0;
    cairo_t *cr;
    PangoLayout *layout;
    double width, height;

    layout =
	get_pango_layout(txt, fontdescription, fontsize, &width, &height);
    //create the right size canvas for character set
    surface =
	cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int) width,
				   (int) height);
//      surface =cairo_image_surface_create(CAIRO_FORMAT_A8,(int)width,(int)height);

    cr = cairo_create(surface);
    //draw a rectangle with same size of canvas
//    cairo_rectangle(cr, 5, 5, width*1.8,height*1.8);
    //fill rectangle with black
//    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5,0.3);
//    cairo_fill(cr);
    //set pen color to white
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    //draw the text
    pango_cairo_show_layout(cr, layout);



    *w = (int) width;
    *h = (int) height;
    g_object_unref(layout);
    cairo_destroy(cr);

    return cairo_image_surface_get_data(surface);
}
