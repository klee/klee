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

#import "GVExportViewController.h"
#import "GVGraph.h"

NSMutableArray *_formatRenders = nil;

@implementation GVExportViewController

@synthesize filename = _filename;
@synthesize render = _render;

+ (void)initialize
{
	if (!_formatRenders) {
		_formatRenders = [[NSMutableArray alloc] init];
		
		NSString *lastFormat = nil;
		NSMutableArray *lastRenders = nil;
		for (NSString *device in [GVGraph pluginsWithAPI:API_device]) {
			NSArray *deviceComponents = [device componentsSeparatedByString:@":"];
			NSUInteger componentCount = [deviceComponents count];
			
			if (componentCount > 0) {
				NSString *format = [deviceComponents objectAtIndex:0];
				if (![lastFormat isEqualToString:format]) {
					lastFormat = format;
					lastRenders = [NSMutableArray array];
					[_formatRenders addObject:[NSDictionary dictionaryWithObjectsAndKeys:lastFormat, @"format", lastRenders, @"renders", nil]];
				}
			}
			
			if (componentCount > 1)
				[lastRenders addObject:[deviceComponents objectAtIndex:1]];
		}
	}
}

- (id)init
{
	if (self = [super initWithNibName:@"Export" bundle:nil]) {
		_panel = nil;
		_filename = nil;
		
		_formatRender = nil;
		_render = nil;
		for (NSDictionary *formatRender in _formatRenders)
			if ([[formatRender objectForKey:@"format"] isEqualToString:@"pdf"]) {
				_formatRender = [formatRender retain];
				if ([[formatRender objectForKey:@"renders"] containsObject:@"quartz"])
					_render = @"quartz";
				break;
			}
	}
	return self;
}

- (NSArray *)formatRenders
{
	return _formatRenders;
}

- (NSString *)device
{
	NSString *format = [_formatRender objectForKey:@"format"];
	return _render ? [NSString stringWithFormat:@"%@:%@", format, _render] : format;
}

- (NSDictionary *)formatRender
{
	return _formatRender;
}

- (void)setFormatRender:(NSDictionary *)formatRender
{
	if (_formatRender != formatRender) {
		[_formatRender release];
		_formatRender = [formatRender retain];
		
		/* force save panel to use this file type */
		[_panel setRequiredFileType:[_formatRender objectForKey:@"format"]];
		
		/* remove existing render if it's not compatible with format */
		if (![[_formatRender objectForKey:@"renders"] containsObject:_render])
			[self setRender:nil];
	}
}

- (void)beginSheetModalForWindow:(NSWindow *)window modalDelegate:(id)modalDelegate didEndSelector:(SEL)selector
{
	/* remember to invoke end selector on the modal delegate */
	NSInvocation *endInvocation = [NSInvocation invocationWithMethodSignature:[modalDelegate methodSignatureForSelector:selector]];
	[endInvocation setTarget:modalDelegate];
	[endInvocation setSelector:selector];
	[endInvocation setArgument:&self atIndex:2];
	[endInvocation retain];
	
	_panel = [NSSavePanel savePanel];
	[_panel setAccessoryView:[self view]];
	[_panel setRequiredFileType:[_formatRender objectForKey:@"format"]];
	[_panel
		beginSheetForDirectory:[_filename stringByDeletingLastPathComponent]
		file:[_filename lastPathComponent]
		modalForWindow:window
		modalDelegate:self
		didEndSelector:@selector(savePanelDidEnd:returnCode:contextInfo:)
		contextInfo:endInvocation];
}

- (void)savePanelDidEnd:(NSSavePanel *)savePanel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	NSInvocation *endInvocation = (NSInvocation *)contextInfo;
	if (returnCode == NSOKButton) {
		NSString *filename = [savePanel filename];
		if (_filename != filename) {
			[_filename release];
			_filename = [filename retain];
		}
		
		/* invoke the end selector on the modal delegate */
		[endInvocation invoke];
	}
	
	[endInvocation release];
	[_panel setAccessoryView:nil];
	_panel = nil;
}

- (void)dealloc
{
	[_panel release];
	[_filename release];
	[_formatRender release];
	[_render release];
	[super dealloc];
}

@end
