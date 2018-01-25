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

#include "gvplugin_device.h"
#include <IL/il.h>
#include <IL/ilu.h>

static void
Y_inv ( unsigned int width, unsigned int height, char *data)
{
        unsigned int x, y, *a, *b, t;

	a = (unsigned int*)data;
        b = a + (height-1) * width;
        for (y = 0; y < height/2; y++) {
                for (x = 0; x < width; x++) {
			t = *a;
			*a++ = *b;
			*b++ = t;
                }
                b -= 2*width;
        }
}

static void devil_format(GVJ_t * job)
{
    ILuint	ImgId;
    ILenum	Error;
    ILboolean rc;

    // Check if the shared lib's version matches the executable's version.
    if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION ||
    	iluGetInteger(ILU_VERSION_NUM) < ILU_VERSION) {
    	fprintf(stderr, "DevIL version is different...exiting!\n");
    }

    // Initialize DevIL.
    ilInit();

    // Generate the main image name to use.
    ilGenImages(1, &ImgId);

    // Bind this image name.
    ilBindImage(ImgId);

    // cairo's inmemory image format needs inverting for DevIL 
    Y_inv ( job->width, job->height, job->imagedata );
    
    // let the DevIL do its thing
    rc = ilTexImage( job->width, job->height,
    		1,		// Depth
    		4,		// Bpp
    		IL_BGRA,	// Format
    		IL_UNSIGNED_BYTE,// Type
    		job->imagedata);
    
    // output to the provided open file handle
    ilSaveF(job->device.id, job->output_file);
    
    // We're done with the image, so delete it.
    ilDeleteImages(1, &ImgId);
    
    // Simple Error detection loop that displays the Error to the user in a human-readable form.
    while ((Error = ilGetError())) {
    	fprintf(stderr, "Error: %s\n", iluErrorString(Error));
    }
}

static gvdevice_engine_t devil_engine = {
    NULL,		/* devil_initialize */
    devil_format,
    NULL,		/* devil_finalize */
};

static gvdevice_features_t device_features_devil = {
	GVDEVICE_BINARY_FORMAT        
          | GVDEVICE_NO_WRITER
          | GVDEVICE_DOES_TRUECOLOR,/* flags */
	{0.,0.},                    /* default margin - points */
	{0.,0.},                    /* default page width, height - points */
	{96.,96.},                  /* svg 72 dpi */
};

gvplugin_installed_t gvdevice_devil_types[] = {
    {IL_BMP, "bmp:cairo", -1, &devil_engine, &device_features_devil},
    {IL_JPG, "jpg:cairo", -1, &devil_engine, &device_features_devil},
    {IL_JPG, "jpe:cairo", -1, &devil_engine, &device_features_devil},
    {IL_JPG, "jpeg:cairo", -1, &devil_engine, &device_features_devil},
    {IL_PNG, "png:cairo", -1, &devil_engine, &device_features_devil},
    {IL_TIF, "tif:cairo", -1, &devil_engine, &device_features_devil},
    {IL_TIF, "tiff:cairo", -1, &devil_engine, &device_features_devil},
    {IL_TGA, "tga:cairo", -1, &devil_engine, &device_features_devil},
    {0, NULL, 0, NULL, NULL}
};
