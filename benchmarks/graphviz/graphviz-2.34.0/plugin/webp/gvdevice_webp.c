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
#include "gvio.h"

#ifdef HAVE_WEBP
#include "webp/encode.h"

static const char* const kErrorMessages[] = {
    "OK",
    "OUT_OF_MEMORY: Out of memory allocating objects",
    "BITSTREAM_OUT_OF_MEMORY: Out of memory re-allocating byte buffer",
    "NULL_PARAMETER: NULL parameter passed to function",
    "INVALID_CONFIGURATION: configuration is invalid",
    "BAD_DIMENSION: Bad picture dimension. Maximum width and height "
	"allowed is 16383 pixels.",
    "PARTITION0_OVERFLOW: Partition #0 is too big to fit 512k.\n"
        "To reduce the size of this partition, try using less segments "
        "with the -segments option, and eventually reduce the number of "
        "header bits using -partition_limit. More details are available "
        "in the manual (`man cwebp`)",
    "PARTITION_OVERFLOW: Partition is too big to fit 16M",
    "BAD_WRITE: Picture writer returned an I/O error",
    "FILE_TOO_BIG: File would be too big to fit in 4G",
    "USER_ABORT: encoding abort requested by user"
};

typedef enum {
    FORMAT_WEBP,
} format_type;

static int writer(const uint8_t* data, size_t data_size, const WebPPicture* const pic) {
    return (gvwrite((GVJ_t *)pic->custom_ptr, (const char *)data, data_size) == data_size) ? 1 : 0;
}

static void webp_format(GVJ_t * job)
{
    WebPPicture picture;
    WebPPreset preset;
    WebPConfig config;
    int stride;

    if (!WebPPictureInit(&picture) || !WebPConfigInit(&config)) {
	fprintf(stderr, "Error! Version mismatch!\n");
	goto Error;
    }

    picture.width = job->width;
    picture.height = job->height;
    stride = 4 * job->width;

    picture.writer = writer;
    picture.custom_ptr = (void*)job;

#if 0
    picture.extra_info_type = 0;
    picture.colorspace = 0;

    config.method = 0;
    config.quality = 0;
    config.show_compressed = 0;
    config.alpha_quality = 0;
    config.alpha_compression = 0;
    config.alpha_filtering = 0;
    config.target_size = 0;
    config.target_PSNR = 0;
    config.sns_strength = 0;
    config.filter_strength = 0;
    config.autofilter = 0;
    config.filter_type = 0;
    config.filter_sharpness = 0;
    config.pass = 0;
    config.preprocessing = 0;
    config.segments = 0;
    config.partition_limit = 0;
#endif

#if 1
    preset = WEBP_PRESET_DRAWING;

    if (!WebPConfigPreset(&config, preset, config.quality)) {
	fprintf(stderr, "Error! Could initialize configuration with preset.\n");
	goto Error;
    }
#endif

#if 1
    if (!WebPValidateConfig(&config)) {
	fprintf(stderr, "Error! Invalid configuration.\n");
	goto Error;
    }
#endif

    if (!WebPPictureAlloc(&picture)) {
	fprintf(stderr, "Error! Cannot allocate memory\n");
	return;
    }

    if (!WebPPictureImportBGRA(&picture,
		(const uint8_t * const)job->imagedata, stride)) {
	fprintf(stderr, "Error! Cannot import picture\n");
	goto Error;
    }

    if (!WebPEncode(&config, &picture)) {
	fprintf(stderr, "Error! Cannot encode picture as WebP\n");
	fprintf(stderr, "Error code: %d (%s)\n",
	    picture.error_code, kErrorMessages[picture.error_code]);
	goto Error;
     }

Error:
     WebPPictureFree(&picture);
}

static gvdevice_engine_t webp_engine = {
    NULL,		/* webp_initialize */
    webp_format,
    NULL,		/* webp_finalize */
};

static gvdevice_features_t device_features_webp = {
	GVDEVICE_BINARY_FORMAT        
          | GVDEVICE_NO_WRITER
          | GVDEVICE_DOES_TRUECOLOR,/* flags */
	{0.,0.},                    /* default margin - points */
	{0.,0.},                    /* default page width, height - points */
	{96.,96.},                  /* 96 dpi */
};
#endif

gvplugin_installed_t gvdevice_webp_types[] = {
#ifdef HAVE_WEBP
    {FORMAT_WEBP, "webp:cairo", 1, &webp_engine, &device_features_webp},
#endif
    {0, NULL, 0, NULL, NULL}
};
