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

#import <UIKit/UIKit.h>

@interface GVTextLayout : NSObject
{
	UIFont* _font;
	NSString* _text;
}

- (id)initWithFontName:(char*)fontName fontSize:(CGFloat)fontSize text:(char*)text;

- (void)sizeUpWidth:(double*)width height:(double*)height yoffset:(double*)yoffset;
- (void)drawInContext:(CGContextRef)context atPosition:(CGPoint)position;

- (void)dealloc;

@end
