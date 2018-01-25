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

#include "types.h"
#include "gvcjob.h"

#include "gvplugin_quartz.h"

#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 20000

#import "GVTextLayout.h"

void *quartz_new_layout(char* fontname, double fontsize, char* text)
{
	return [[GVTextLayout alloc] initWithFontName:fontname fontSize:fontsize text:text];
}

void quartz_size_layout(void *layout, double* width, double* height, double* yoffset_layout)
{
	[(GVTextLayout*)layout sizeUpWidth:width height:height yoffset:yoffset_layout];
}

void quartz_draw_layout(void *layout, CGContextRef context, CGPoint position)
{
	[(GVTextLayout*)layout drawInContext:context atPosition:position];	
}

void quartz_free_layout(void *layout)
{
	[(GVTextLayout*)layout release];
}

static NSString* _defaultFontName = @"TimesNewRomanPSMT";

@implementation GVTextLayout

- (id)initWithFontName:(char*)fontName fontSize:(CGFloat)fontSize text:(char*)text
{
	if (self = [super init])
	{
		_font = nil;
		if (fontName)
			_font = [[UIFont fontWithName:[NSString stringWithUTF8String:fontName] size:fontSize] retain];
		if (!_font)
			_font = [[UIFont fontWithName:_defaultFontName size:fontSize] retain];
			
		_text = text ? [[NSString alloc] initWithUTF8String:text] : nil;
	}
	return self;
}

- (void)sizeUpWidth:(double*)width height:(double*)height yoffset:(double*)yoffset
{
	CGSize size = [_text sizeWithFont:_font];
	CGFloat ascender = _font.ascender;
	
	*width = size.width;
	*height = size.height;
	*yoffset = ascender;
}

- (void)drawAtPoint:(CGPoint)point inContext:(CGContextRef)context
{
	UIGraphicsPushContext(context);
	[_text drawAtPoint:point withFont:_font];
	UIGraphicsPopContext();
}

- (void)drawInContext:(CGContextRef)context atPosition:(CGPoint)position
{
	UIGraphicsPushContext(context);
	[_text drawAtPoint:position withFont:_font];
	UIGraphicsPopContext();	
}

- (void)dealloc
{
	[_font release];
	[_text release];
	
	[super dealloc];
}


@end

#endif
