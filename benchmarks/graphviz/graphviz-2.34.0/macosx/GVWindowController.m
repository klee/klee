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

#import "GVWindowController.h"
#import "GVGraph.h"
#import "GVDocument.h"

@implementation GVWindowController

- (id)init
{
	if (self = [super initWithWindowNibName: @"Document"])
		;
	return self;
}

- (void)setDocument: (NSDocument *)document
{
			NSNotificationCenter* defaultCenter = [NSNotificationCenter defaultCenter];
	
	GVDocument *oldDocument = [self document];
	if (oldDocument)
		[defaultCenter removeObserver:self name:@"GVGraphDocumentDidChange" object:oldDocument];
	[super setDocument:document];
	[defaultCenter addObserver:self selector:@selector(graphDocumentDidChange:) name:@"GVGraphDocumentDidChange" object:document];
}

- (void)awakeFromNib
{
	[self graphDocumentDidChange:nil];
	
	/* if window is not at standard size, make it standard size */
	NSWindow *window = [self window];
	if (![window isZoomed])
		[window zoom:self];
}

- (void)graphDocumentDidChange:(NSNotification*)notification
{
	/* whenever the graph changes, rerender its PDF and display that */
	GVDocument *document = [self document];
	if ([document respondsToSelector:@selector(graph)])
		[documentView setDocument:[[[PDFDocument alloc] initWithData:[[document graph] renderWithFormat:@"pdf:quartz"]] autorelease]];
}

- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)defaultFrame
{
	/* standard size for zooming is whatever will fit the content exactly */
	NSRect currentFrame = [window frame];
	NSRect standardFrame = [window frameRectForContentRect:[[documentView documentView] bounds]];
	standardFrame.origin.x = currentFrame.origin.x;
	standardFrame.origin.y = currentFrame.origin.y + currentFrame.size.height - standardFrame.size.height;
	return standardFrame;
}

- (IBAction)actualSizeView:(id)sender
{
	[documentView setScaleFactor:1.0];
}

- (IBAction)zoomInView:(id)sender
{
	[documentView zoomIn:sender];
}

- (IBAction)zoomOutView:(id)sender
{
	[documentView zoomOut:sender];
}

- (IBAction)zoomToFitView:(id)sender
{
	[documentView setAutoScales:YES];
	[documentView setAutoScales:NO];
}

- (IBAction)printGraphDocument:(id)sender
{
	[documentView printWithInfo:[[self document] printInfo] autoRotate:NO];
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
	/* validate toolbar or menu items */
	if ([anItem action] == @selector(actualSizeView:))
		return YES;
	else if ([anItem action] == @selector(zoomInView:))
		return [documentView canZoomIn];
	else if ([anItem action] == @selector(zoomOutView:))
		return [documentView canZoomOut];
	else if ([anItem action] == @selector(zoomToFitView:))
		return YES;
	else if ([anItem action] == @selector(printGraphDocument:))
		return YES;
	else
		return NO;
}
- (void)dealloc
{
    char darwinStr[256];
    size_t size = sizeof(darwinStr);
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"GVGraphDocumentDidChange" object:[self document]];
    sysctlbyname("kern.osrelease", darwinStr, &size, NULL, 0);
    NSString *darwinVer = [NSString stringWithCString:darwinStr encoding:NSASCIIStringEncoding]; 
    NSString *baseVer = @"11";
    if ([darwinVer compare:baseVer] < 0)
         [super dealloc];
}


@end
