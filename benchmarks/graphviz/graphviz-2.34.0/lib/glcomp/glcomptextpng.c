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

#include "glcompfont.h"
#include "memory.h"
#include <gtk/gtk.h>
#include <png.h>

unsigned char *glCompLoadPng (char *filename, int *imageWidth, int *imageHeight)
{
    cairo_surface_t *surface;
    cairo_format_t format;
    int w, h;
    unsigned char *d;
    surface = NULL;

    surface = cairo_image_surface_create_from_png(filename);
    if (!surface) return 0;
    w = cairo_image_surface_get_width(surface);
    h = cairo_image_surface_get_height(surface);
    *imageWidth = w;
    *imageHeight = h;
    format = cairo_image_surface_get_format(surface);
    d = cairo_image_surface_get_data(surface);
    return d;
}

#if 0

unsigned char *load_raw(char *filename, int width, int height)
{
    unsigned char *data;
    FILE *file;
    // allocate buffer
    data = N_NEW(width * height * 3, unsigned char);
    // open and read texture data
    file = fopen(filename, "rb");
    fread(data, width * height * 3, 1, file);
    fclose (file);
    return data;
}

unsigned char *load_png2(char *file_name, int *imageWidth,
			 int *imageHeight)
{
    unsigned char *imageData = NULL;
    unsigned char header[8];
    int i, ii, b0, b1, b2, b3, pixeloffset;
    long int c;
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    png_bytepp row_pointers;	//actual image data
    int is_png = 0;
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
	return (unsigned char *) 0;
    }
    fread(header, 1, 8, fp);
    is_png = !png_sig_cmp(header, 0, 8);
    if (!is_png) {
	fclose (fp);
	printf("glcomp error:file is not a valid PNG file\n");
	return (unsigned char *) 0;
    }

    png_ptr = png_create_read_struct
	(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
	fclose (fp);
	printf("glcomp error:file can not be read\n");
	return (unsigned char *) 0;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
	png_destroy_read_struct(&png_ptr,
				(png_infopp) NULL, (png_infopp) NULL);
	fclose (fp);
	printf("glcomp error:PNG file header is corrupted\n");
	return (unsigned char *) 0;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
	printf("glcomp error:PNG file header is corrupted\n");
	fclose (fp);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
	return (unsigned char *) 0;
    }


    png_init_io(png_ptr, fp);

    png_set_sig_bytes(png_ptr, 8);	//pass signature bytes
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);	//read real image data

    row_pointers = png_malloc(png_ptr,
			      info_ptr->height * sizeof(png_bytepp));
    row_pointers = png_get_rows(png_ptr, info_ptr);
    *imageWidth = info_ptr->width;
    *imageHeight = info_ptr->height;
    imageData = N_NEW(info_ptr->height * info_ptr->width, unsigned char);
    c = 0;
    //decide what pixel offset to use, ro
    pixeloffset = png_get_rowbytes(png_ptr, info_ptr) / info_ptr->width;

    b0 = -1;
    b1 = -1;
    b2 = -1;
    b3 = -1;

    for (i = 0; i < (int) info_ptr->height; i++) {
	for (ii = 0; ii < (int) png_get_rowbytes(png_ptr, info_ptr);
	     ii = ii + pixeloffset) {
	    imageData[c] = row_pointers[info_ptr->height - i - 1][ii];
	    if ((b0 != row_pointers[info_ptr->height - i - 1][ii])
		|| (b1 != row_pointers[info_ptr->height - i - 1][ii + 1])
		|| (b2 != row_pointers[info_ptr->height - i - 1][ii + 2])
		|| (b3 !=
		    row_pointers[info_ptr->height - i - 1][ii + 3])) {
		b0 = row_pointers[info_ptr->height - i - 1][ii];
		b1 = row_pointers[info_ptr->height - i - 1][ii + 1];
		b2 = row_pointers[info_ptr->height - i - 1][ii + 2];
		b3 = row_pointers[info_ptr->height - i - 1][ii + 3];
	    }
	    c++;
	}
    }
    //cleaning libpng mess
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    png_free(png_ptr, row_pointers);

    fclose (fp);
    return imageData;
}
#endif


