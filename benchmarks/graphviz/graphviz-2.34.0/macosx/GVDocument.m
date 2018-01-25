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

#import "GVDocument.h"
#import "GVExportViewController.h"
#import "GVFileNotificationCenter.h"
#import "GVGraph.h"
#import "GVWindowController.h"

@implementation GVDocument

@synthesize graph = _graph;

- (id)init
{
	if (self = [super init]) {
		_exporter = nil;
		_graph = nil;
	}
    return self;
}

- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
	[_graph release];
	_graph = [[GVGraph alloc] initWithURL:absoluteURL error:outError];
	[_graph.arguments setValue:@"dot" forKey:@"layout"];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(graphDidChange:) name:@"GVGraphDidChange" object:_graph];
	[[GVFileNotificationCenter defaultCenter] addObserver:self selector:@selector(fileDidChange:) path:[absoluteURL path]];
	
	return _graph != nil;
}

- (BOOL)writeToURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError
{
	return [_graph writeToURL:absoluteURL error:outError];
}

- (void)makeWindowControllers
{
	[self addWindowController: [[[GVWindowController alloc] init] autorelease]];
}

- (void)setPrintInfo:(NSPrintInfo *)printInfo
{
	/* after Page Setup is run, change the page size and margins of the graph to fit the page setup parameters */
	[super setPrintInfo:printInfo];
	NSSize paperSize = [printInfo paperSize];
	NSRect imageablePageBounds = [printInfo imageablePageBounds];
	double scalingFactor = 72.0 * [[[printInfo dictionary] objectForKey:NSPrintScalingFactor] doubleValue];
	
	[_graph.graphAttributes setObject:[NSString stringWithFormat:@"%f,%f",
		paperSize.width / scalingFactor,
		paperSize.height / scalingFactor]
		forKey:@"page"];
	[_graph.graphAttributes setObject:[NSString stringWithFormat:@"%f,%f",
		fmax(imageablePageBounds.origin.x, paperSize.width - imageablePageBounds.size.width - imageablePageBounds.origin.x) / scalingFactor,
		fmax(imageablePageBounds.origin.y, paperSize.height - imageablePageBounds.size.height - imageablePageBounds.origin.y) / scalingFactor]
		forKey:@"margin"];
}

- (IBAction)exportDocument:(id)sender
{
	if (!_exporter) {
		_exporter = [[GVExportViewController alloc] init];
		[_exporter setFilename:[[[self fileURL] path] stringByDeletingPathExtension]];
	}
	[_exporter beginSheetModalForWindow:[self windowForSheet] modalDelegate:self didEndSelector:@selector(exporterDidEnd:)];
}

- (void)exporterDidEnd:(GVExportViewController *)exporter
{
	[_graph renderWithFormat:[exporter device] toURL:[NSURL fileURLWithPath:[exporter filename]]];
}

- (void)fileDidChange:(NSString *)path
{
	NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
	[defaultCenter removeObserver:self name:@"GVGraphDidChange" object:_graph];
	
	/* reparse the graph fresh from the file */
	[_graph release];
	_graph = [[GVGraph alloc] initWithURL:[self fileURL] error:nil];
	[_graph.arguments setValue:@"dot" forKey:@"layout"];
	[defaultCenter addObserver:self selector:@selector(graphDidChange:) name:@"GVGraphDidChange" object:_graph];
	
	[defaultCenter postNotificationName:@"GVGraphDocumentDidChange" object:self];
}

- (void)graphDidChange:(NSNotification *)notification
{
	[[NSNotificationCenter defaultCenter] postNotificationName:@"GVGraphDocumentDidChange" object:self];
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:@"GVGraphDidChange" object:_graph];
	[[GVFileNotificationCenter defaultCenter] removeObserver:self path:[[self fileURL] path]];

	[_exporter release];
	[_graph release];
	[super dealloc];
}

@end
