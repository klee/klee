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

#import <Cocoa/Cocoa.h>

@interface GVExportViewController : NSViewController
{
	NSSavePanel *_panel;
	NSString *_filename;
	NSDictionary *_formatRender;
	NSString *_render;
}

@property(readonly) NSArray *formatRenders;
@property(readonly) NSString *device;
@property(retain) NSString *filename;
@property(retain) NSDictionary *formatRender;
@property(retain) NSString *render;

- (id)init;

- (void)beginSheetModalForWindow:(NSWindow *)window modalDelegate:(id)modalDelegate didEndSelector:(SEL)selector;

@end
