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

#include <stdlib.h>
#include <string.h>

#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000
#include <mach/mach_host.h>
#include <sys/mman.h>
#endif

#include "gvplugin_device.h"
#include "gvplugin_render.h"
#ifdef WITH_CGRAPH
#include "cgraph.h"
#else
#include "graph.h"
#endif

#include "gvplugin_quartz.h"

static CGFloat dashed[] = { 6.0 };
static CGFloat dotted[] = { 2.0, 6.0 };

static void quartzgen_begin_job(GVJ_t * job)
{
    if (!job->external_context)
	job->context = NULL;
    else if (job->device.id == FORMAT_CGIMAGE) {
	/* save the passed-in context in the window field, so we can create a CGContext in the context field later on */
	job->window = job->context;
	*((CGImageRef *) job->window) = NULL;
	job->context = NULL;
    }
}

static void quartzgen_end_job(GVJ_t * job)
{
    CGContextRef context = (CGContextRef) job->context;
    if (!job->external_context) {
	switch (job->device.id) {

	case FORMAT_PDF:
	    /* save the PDF */
	    CGPDFContextClose(context);
	    break;

	case FORMAT_CGIMAGE:
	    break;

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040
	default:		/* bitmap formats */
	    {
		/* create an image destination */
		CGDataConsumerRef data_consumer =
		    CGDataConsumerCreate(job,
					 &device_data_consumer_callbacks);
		CGImageDestinationRef image_destination =
		    CGImageDestinationCreateWithDataConsumer(data_consumer,
							     format_uti
							     [job->device.
							      id], 1,
							     NULL);

		/* add the bitmap image to the destination and save it */
		CGImageRef image = CGBitmapContextCreateImage(context);
		CGImageDestinationAddImage(image_destination, image, NULL);
		CGImageDestinationFinalize(image_destination);

		/* clean up */
		if (image_destination)
		    CFRelease(image_destination);
		CGImageRelease(image);
		CGDataConsumerRelease(data_consumer);
	    }
	    break;
#endif
	}
	CGContextRelease(context);
    } else if (job->device.id == FORMAT_CGIMAGE) {
	/* create an image and save it where the window field is, which was set to the passed-in context at begin job */
	*((CGImageRef *) job->window) =
	    CGBitmapContextCreateImage(context);
#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000
	void *context_data = CGBitmapContextGetData(context);
	size_t context_datalen =
	    CGBitmapContextGetBytesPerRow(context) *
	    CGBitmapContextGetHeight(context);
#endif
	CGContextRelease(context);
#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000
	munmap(context_data, context_datalen);
#endif
    }
}

static void quartzgen_begin_page(GVJ_t * job)
{
    CGRect bounds = CGRectMake(0.0, 0.0, job->width, job->height);

    if (!job->context) {

	switch (job->device.id) {

	case FORMAT_PDF:
	    {
		/* create the auxiliary info for PDF content, author and title */
		CFStringRef auxiliaryKeys[] = {
		    kCGPDFContextCreator,
		    kCGPDFContextTitle
		};
		CFStringRef auxiliaryValues[] = {
		    CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
					     CFSTR("%s %s"),
					     job->common->info[0],
					     job->common->info[1]),
		    job->obj->type ==
			ROOTGRAPH_OBJTYPE ?
			CFStringCreateWithBytesNoCopy(kCFAllocatorDefault,
						      (const UInt8 *) agnameof(job->obj->u.g),
						      strlen(agnameof(job->obj->u.g)),
						      kCFStringEncodingUTF8,
						      false,
						      kCFAllocatorNull)
			: CFSTR("")
		};
		CFDictionaryRef auxiliaryInfo =
		    CFDictionaryCreate(kCFAllocatorDefault,
				       (const void **) &auxiliaryKeys,
				       (const void **) &auxiliaryValues,
				       sizeof(auxiliaryValues) /
				       sizeof(auxiliaryValues[0]),
				       &kCFTypeDictionaryKeyCallBacks,
				       &kCFTypeDictionaryValueCallBacks);

		/* create a PDF for drawing into */
		CGDataConsumerRef data_consumer =
		    CGDataConsumerCreate(job,
					 &device_data_consumer_callbacks);
		job->context =
		    CGPDFContextCreate(data_consumer, &bounds,
				       auxiliaryInfo);

		/* clean up */
		CGDataConsumerRelease(data_consumer);
		CFRelease(auxiliaryInfo);
		int i;
		for (i = 0;
		     i <
		     sizeof(auxiliaryValues) / sizeof(auxiliaryValues[0]);
		     ++i)
		    CFRelease(auxiliaryValues[i]);
	    }
	    break;

	default:		/* bitmap formats */
	    {
		size_t bytes_per_row =
		    (job->width * BYTES_PER_PIXEL +
		     BYTE_ALIGN) & ~BYTE_ALIGN;

		void *buffer = NULL;

#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000

		/* iPhoneOS has no swap files for memory, so if we're short of memory we need to make our own temp scratch file to back it */

		size_t buffer_size = job->height * bytes_per_row;
		mach_msg_type_number_t vm_info_size = HOST_VM_INFO_COUNT;
		vm_statistics_data_t vm_info;

		if (host_statistics
		    (mach_host_self(), HOST_VM_INFO,
		     (host_info_t) & vm_info,
		     &vm_info_size) != KERN_SUCCESS
		    || buffer_size * 2 >
		    vm_info.free_count * vm_page_size) {
		    FILE *temp_file = tmpfile();
		    if (temp_file) {
			int temp_file_descriptor = fileno(temp_file);
			if (temp_file_descriptor >= 0
			    && ftruncate(temp_file_descriptor,
					 buffer_size) == 0) {
			    buffer =
				mmap(NULL, buffer_size,
				     PROT_READ | PROT_WRITE,
				     MAP_FILE | MAP_SHARED,
				     temp_file_descriptor, 0);
			    if (buffer == (void *) -1)
				buffer = NULL;
			}
			fclose(temp_file);
		    }
		}
		if (!buffer)
		    buffer = mmap(NULL,
				  buffer_size,
				  PROT_READ | PROT_WRITE,
				  MAP_ANON | MAP_SHARED, -1, 0);
#endif

		/* create a true color bitmap for drawing into */
		CGColorSpaceRef color_space =
		    CGColorSpaceCreateDeviceRGB();
		job->context = CGBitmapContextCreate(buffer,	/* data: MacOSX lets system allocate, iPhoneOS use manual memory mapping */
						     job->width,	/* width in pixels */
						     job->height,	/* height in pixels */
						     BITS_PER_COMPONENT,	/* bits per component */
						     bytes_per_row,	/* bytes per row: align to 16 byte boundary */
						     color_space,	/* color space: device RGB */
						     kCGImageAlphaPremultipliedFirst	/* bitmap info: premul ARGB has best support in OS X */
		    );
		job->imagedata =
		    CGBitmapContextGetData((CGContextRef) job->context);

		/* clean up */
		CGColorSpaceRelease(color_space);
	    }
	    break;
	}

    }

    /* start the page (if this is a paged context) and graphics state */
    CGContextRef context = (CGContextRef) job->context;
    CGContextBeginPage(context, &bounds);
    CGContextSaveGState(context);
    CGContextSetMiterLimit(context, 1.0);
    CGContextSetLineJoin(context, kCGLineJoinRound);

    /* set up the context transformation */
    CGContextScaleCTM(context, job->scale.x, job->scale.y);
    CGContextRotateCTM(context, -job->rotation * M_PI / 180.0);
    CGContextTranslateCTM(context, job->translation.x, job->translation.y);
}

static void quartzgen_end_page(GVJ_t * job)
{
    /* end the page (if this is a paged context) and graphics state */
    CGContextRef context = (CGContextRef) job->context;
    CGContextRestoreGState(context);
    CGContextEndPage(context);
}

static void quartzgen_begin_anchor(GVJ_t * job, char *url, char *tooltip,
				   char *target, char *id)
{
    pointf *url_map = job->obj->url_map_p;
    if (url && url_map) {
	/* set up the hyperlink to the given url */
	CGContextRef context = (CGContextRef) job->context;
	CFURLRef uri =
	    CFURLCreateWithBytes(kCFAllocatorDefault, (const UInt8 *) url,
				 strlen(url), kCFStringEncodingUTF8, NULL);
	CGPDFContextSetURLForRect(context, uri,
				  /* need to reverse the CTM on the area to get it to work */
				  CGRectApplyAffineTransform(CGRectMake
							     (url_map[0].x,
							      url_map[0].y,
							      url_map[1].
							      x -
							      url_map[0].x,
							      url_map[1].
							      y -
							      url_map[0].
							      y),
							     CGContextGetCTM
							     (context))
	    );

	/* clean up */
	CFRelease(uri);
    }
}

static void quartzgen_path(GVJ_t * job, int filled)
{
    CGContextRef context = (CGContextRef) job->context;

    /* set up colors */
    if (filled)
	CGContextSetRGBFillColor(context, job->obj->fillcolor.u.RGBA[0],
				 job->obj->fillcolor.u.RGBA[1],
				 job->obj->fillcolor.u.RGBA[2],
				 job->obj->fillcolor.u.RGBA[3]);
    CGContextSetRGBStrokeColor(context, job->obj->pencolor.u.RGBA[0],
			       job->obj->pencolor.u.RGBA[1],
			       job->obj->pencolor.u.RGBA[2],
			       job->obj->pencolor.u.RGBA[3]);

    /* set up line style */
    const CGFloat *segments;
    size_t segment_count;
    switch (job->obj->pen) {
    case PEN_DASHED:
	segments = dashed;
	segment_count = sizeof(dashed) / sizeof(CGFloat);
	break;
    case PEN_DOTTED:
	segments = dotted;
	segment_count = sizeof(dotted) / sizeof(CGFloat);
	break;
    default:
	segments = NULL;
	segment_count = 0;
	break;
    }
    CGContextSetLineDash(context, 0.0, segments, segment_count);

    /* set up line width */
    CGContextSetLineWidth(context, job->obj->penwidth);	// *job->scale.x);

    /* draw the path */
    CGContextDrawPath(context, filled ? kCGPathFillStroke : kCGPathStroke);
}

void quartzgen_textpara(GVJ_t * job, pointf p, textpara_t * para)
{
    CGContextRef context = (CGContextRef) job->context;

    /* adjust text position */
    switch (para->just) {
    case 'r':
	p.x -= para->width;
	break;
    case 'l':
	p.x -= 0.0;
	break;
    case 'n':
    default:
	p.x -= para->width / 2.0;
	break;
    }
    p.y += para->yoffset_centerline;

    void *layout;
    if (para->free_layout == &quartz_free_layout)
	layout = para->layout;
    else
	layout =
	    quartz_new_layout(para->fontname, para->fontsize, para->str);

#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000
    CGContextSaveGState(context);
    CGContextScaleCTM(context, 1.0, -1.0);
    p.y = -p.y - para->yoffset_layout;
#endif
    CGContextSetRGBFillColor(context, job->obj->pencolor.u.RGBA[0],
			     job->obj->pencolor.u.RGBA[1],
			     job->obj->pencolor.u.RGBA[2],
			     job->obj->pencolor.u.RGBA[3]);
    quartz_draw_layout(layout, context, CGPointMake(p.x, p.y));

#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000
    CGContextRestoreGState(context);
#endif

    if (para->free_layout != &quartz_free_layout)
	quartz_free_layout(layout);
}

static void quartzgen_ellipse(GVJ_t * job, pointf * A, int filled)
{
    /* convert ellipse into the current path */
    CGContextRef context = (CGContextRef) job->context;
    double dx = A[1].x - A[0].x;
    double dy = A[1].y - A[0].y;
    CGContextAddEllipseInRect(context,
			      CGRectMake(A[0].x - dx, A[0].y - dy,
					 dx * 2.0, dy * 2.0));

    /* draw the ellipse */
    quartzgen_path(job, filled);
}

static void quartzgen_polygon(GVJ_t * job, pointf * A, int n, int filled)
{
    /* convert polygon into the current path */
    CGContextRef context = (CGContextRef) job->context;
    CGContextMoveToPoint(context, A[0].x, A[0].y);
    int i;
    for (i = 1; i < n; ++i)
	CGContextAddLineToPoint(context, A[i].x, A[i].y);
    CGContextClosePath(context);

    /* draw the ellipse */
    quartzgen_path(job, filled);
}

static void
quartzgen_bezier(GVJ_t * job, pointf * A, int n, int arrow_at_start,
		 int arrow_at_end, int filled)
{
    /* convert bezier into the current path */
    CGContextRef context = (CGContextRef) job->context;
    CGContextMoveToPoint(context, A[0].x, A[0].y);
    int i;
    for (i = 1; i < n; i += 3)
	CGContextAddCurveToPoint(context, A[i].x, A[i].y, A[i + 1].x,
				 A[i + 1].y, A[i + 2].x, A[i + 2].y);

    /* draw the ellipse */
    quartzgen_path(job, filled);
}

static void quartzgen_polyline(GVJ_t * job, pointf * A, int n)
{
    /* convert polyline into the current path */
    CGContextRef context = (CGContextRef) job->context;
    CGContextMoveToPoint(context, A[0].x, A[0].y);
    int i;
    for (i = 1; i < n; ++i)
	CGContextAddLineToPoint(context, A[i].x, A[i].y);

    /* draw the ellipse */
    quartzgen_path(job, FALSE);
}

static gvrender_engine_t quartzgen_engine = {
    quartzgen_begin_job,
    quartzgen_end_job,
    0,				/* quartzgen_begin_graph */
    0,				/* quartzgen_end_graph */
    0,				/* quartzgen_begin_layer */
    0,				/* quartzgen_end_layer */
    quartzgen_begin_page,
    quartzgen_end_page,
    0,				/* quartzgen_begin_cluster */
    0,				/* quartzgen_end_cluster */
    0,				/* quartzgen_begin_nodes */
    0,				/* quartzgen_end_nodes */
    0,				/* quartzgen_begin_edges */
    0,				/* quartzgen_end_edges */
    0,				/* quartzgen_begin_node */
    0,				/* quartzgen_end_node */
    0,				/* quartzgen_begin_edge */
    0,				/* quartzgen_end_edge */
    quartzgen_begin_anchor,
    0,				/* quartzgen_end_anchor */
    0,				/* quartzgen_begin_label */
    0,				/* quartzgen_end_label */
    quartzgen_textpara,
    0,
    quartzgen_ellipse,
    quartzgen_polygon,
    quartzgen_bezier,
    quartzgen_polyline,
    0,				/* quartzgen_comment */
    0,				/* quartzgen_library_shape */
};

static gvrender_features_t render_features_quartz = {
    GVRENDER_DOES_MAPS | GVRENDER_DOES_MAP_RECTANGLE | GVRENDER_DOES_TRANSFORM,	/* flags */
    4.,				/* default pad - graph units */
    NULL,			/* knowncolors */
    0,				/* sizeof knowncolors */
    RGBA_DOUBLE			/* color_type */
};

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040 || __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000
static gvdevice_features_t device_features_quartz = {
    GVDEVICE_BINARY_FORMAT | GVDEVICE_DOES_TRUECOLOR,	/* flags */
    {0., 0.},			/* default margin - points */
    {0., 0.},			/* default page width, height - points */
    {96., 96.}			/* dpi */
};
#endif

static gvdevice_features_t device_features_quartz_paged = {
    GVDEVICE_DOES_PAGES | GVDEVICE_BINARY_FORMAT | GVDEVICE_DOES_TRUECOLOR | GVRENDER_NO_WHITE_BG,	/* flags */
    {36., 36.},			/* default margin - points */
    {0., 0.},			/* default page width, height - points */
    {72., 72.}			/* dpi */
};

gvplugin_installed_t gvrender_quartz_types[] = {
    {0, "quartz", 1, &quartzgen_engine, &render_features_quartz},
    {0, NULL, 0, NULL, NULL}
};

gvplugin_installed_t gvdevice_quartz_types[] = {
    {FORMAT_PDF, "pdf:quartz", 8, NULL, &device_features_quartz_paged},
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040 || __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000
    {FORMAT_CGIMAGE, "cgimage:quartz", 8, NULL, &device_features_quartz},
#endif
#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040
    {FORMAT_BMP, "bmp:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_GIF, "gif:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_EXR, "exr:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_JPEG, "jpe:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_JPEG, "jpeg:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_JPEG, "jpg:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_JPEG2000, "jp2:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_PICT, "pct:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_PICT, "pict:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_PNG, "png:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_PSD, "psd:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_SGI, "sgi:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_TIFF, "tif:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_TIFF, "tiff:quartz", 8, NULL, &device_features_quartz},
    {FORMAT_TGA, "tga:quartz", 8, NULL, &device_features_quartz},
#endif
    {0, NULL, 0, NULL, NULL}
};
