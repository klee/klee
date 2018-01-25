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

#include "gvplugin_textlayout.h"
#include "gvplugin_quartz.h"

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1050

void *quartz_new_layout(char* fontname, double fontsize, char* text)
{
	CFStringRef fontnameref = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)fontname, strlen(fontname), kCFStringEncodingUTF8, FALSE);
	CFStringRef textref = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)text, strlen(text), kCFStringEncodingUTF8, FALSE);
	CTLineRef line = NULL;
	
	if (fontnameref && textref) {
		/* set up the Core Text line */
		CTFontRef font = CTFontCreateWithName(fontnameref, fontsize, NULL);
		
		CFDictionaryRef attributes = CFDictionaryCreate(
			kCFAllocatorDefault,
			(const void**)&kCTFontAttributeName,
			(const void**)&font,
			1,
			&kCFTypeDictionaryKeyCallBacks,
			&kCFTypeDictionaryValueCallBacks);
		CFAttributedStringRef attributed = CFAttributedStringCreate(kCFAllocatorDefault, textref, attributes);
		line = CTLineCreateWithAttributedString(attributed);
		
		CFRelease(attributed);
		CFRelease(attributes);
		CFRelease(font);
	}
	
	if (textref)
		CFRelease(textref);
	if (fontnameref)
		CFRelease(fontnameref);
	return (void *)line;
}

void quartz_size_layout(void *layout, double* width, double* height, double* yoffset_layout)
{
	/* get the typographic bounds */
	CGFloat ascent = 0.0;
	CGFloat descent = 0.0;
	CGFloat leading = 0.0;
	double typowidth = CTLineGetTypographicBounds((CTLineRef)layout, &ascent, &descent, &leading);
	CGFloat typoheight = ascent + descent;
	
	*width = typowidth;
	*height = leading == 0.0 ? typoheight * 1.2 : typoheight + leading;	/* if no leading, use 20% of height */
	*yoffset_layout = ascent;
}

void quartz_draw_layout(void *layout, CGContextRef context, CGPoint position)
{
	CGContextSetTextPosition(context, position.x, position.y);
	
	CFArrayRef runs = CTLineGetGlyphRuns((CTLineRef)layout);
	CFIndex run_count = CFArrayGetCount(runs);
	CFIndex run_index;
	for (run_index = 0; run_index < run_count; ++run_index)
	{
		CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, run_index);
		CTFontRef run_font = CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);
		CGFontRef glyph_font = CTFontCopyGraphicsFont(run_font, NULL);
		CFIndex glyph_count = CTRunGetGlyphCount(run);
		CGGlyph glyphs[glyph_count];
		CGPoint positions[glyph_count];
		CFRange everything = CFRangeMake(0, 0);
		CTRunGetGlyphs(run, everything, glyphs);
		CTRunGetPositions(run, everything, positions);
		
		CGContextSetFont(context, glyph_font);
		CGContextSetFontSize(context, CTFontGetSize(run_font));
		CGContextShowGlyphsAtPositions(context, glyphs, positions, glyph_count);
		
		CGFontRelease(glyph_font);
	}
}

void quartz_free_layout(void *layout)
{
	if (layout)
		CFRelease((CTLineRef)layout);
};

#endif

boolean quartz_textlayout(textpara_t *para, char **fontpath)
{
	void *line = quartz_new_layout(para->fontname, para->fontsize, para->str);
	if (line)
	{
		/* report the layout */
		para->layout = (void*)line;
		para->free_layout = &quartz_free_layout;
		para->yoffset_centerline = 0;
		quartz_size_layout((void*)line, &para->width, &para->height, &para->yoffset_layout);
		return TRUE;
	}
	else
		return FALSE;
};

static gvtextlayout_engine_t quartz_textlayout_engine = {
    quartz_textlayout
};

gvplugin_installed_t gvtextlayout_quartz_types[] = {
    {0, "textlayout", 8, &quartz_textlayout_engine, NULL},
    {0, NULL, 0, NULL, NULL}
};
