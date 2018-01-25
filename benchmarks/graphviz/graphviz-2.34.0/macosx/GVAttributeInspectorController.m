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

#import "GVAttributeInspectorController.h"
#import "GVAttributeSchema.h"
#import "GVDocument.h"
#import "GVGraph.h"
#import "GVWindowController.h"

@implementation GVAttributeInspectorController

- (id)init
{
	if (self = [super initWithWindowNibName: @"Attributes"]) {
		_allSchemas = nil;
		_allAttributes = [[NSMutableDictionary alloc] init];
		_inspectedDocument = nil;
		_otherChangedGraph = YES;
	}
	return self;
}

- (void)awakeFromNib
{
	/* set component toolbar */
	[_allSchemas release];
	_allSchemas = [[NSDictionary alloc] initWithObjectsAndKeys:
		[GVAttributeSchema attributeSchemasWithComponent:@"graph"], [graphToolbarItem itemIdentifier],
		[GVAttributeSchema attributeSchemasWithComponent:@"node"], [nodeDefaultToolbarItem itemIdentifier],
		[GVAttributeSchema attributeSchemasWithComponent:@"edge"], [edgeDefaultToolbarItem itemIdentifier],
		nil];
	[componentToolbar setSelectedItemIdentifier:[graphToolbarItem itemIdentifier]];
	[self toolbarItemDidSelect:nil];
		
	/* start observing whenever a window becomes main */
	[self graphWindowDidBecomeMain:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(graphWindowDidBecomeMain:) name:NSWindowDidBecomeMainNotification object:nil];
}

- (IBAction)toolbarItemDidSelect:(id)sender
{
	/* reload the table */
	[attributeTable reloadData];
}

- (void)graphWindowDidBecomeMain:(NSNotification *)notification
{
	NSWindow* mainWindow = notification ? [notification object] : [NSApp mainWindow];
	GVDocument* mainWindowDocument = [[mainWindow windowController] document];
		
	/* update and observe referenced document */
			NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
	if (_inspectedDocument)
		[defaultCenter removeObserver:self name:@"GVGraphDocumentDidChange" object:_inspectedDocument];
	_inspectedDocument = mainWindowDocument;
	[defaultCenter addObserver:self selector:@selector(graphDocumentDidChange:) name:@"GVGraphDocumentDidChange" object:mainWindowDocument];

	[self reloadAttributes];
		
			/* update the UI */
			[[self window] setTitle:[NSString stringWithFormat:@"%@ Attributes", [mainWindow title]]];
			[attributeTable reloadData];
}

- (void)graphDocumentDidChange:(NSNotification *)notification
{
	/* if we didn't instigate the change, update the UI */
	if (_otherChangedGraph) {
		[self reloadAttributes];
		[attributeTable reloadData];
	}
}

- (void)reloadAttributes
{
	/* reload the attributes from the inspected document's graph */
	[_allAttributes removeAllObjects];
	if ([_inspectedDocument respondsToSelector:@selector(graph)]) {
		GVGraph *graph = [_inspectedDocument graph];
		[_allAttributes setObject:graph.graphAttributes forKey:[graphToolbarItem itemIdentifier]];
		[_allAttributes setObject:graph.defaultNodeAttributes forKey:[nodeDefaultToolbarItem itemIdentifier]];
		[_allAttributes setObject:graph.defaultEdgeAttributes forKey:[edgeDefaultToolbarItem itemIdentifier]];
	}
}

- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar
{
	/* which toolbar items are selectable */
	return [NSArray arrayWithObjects:
		[graphToolbarItem itemIdentifier],
		[nodeDefaultToolbarItem itemIdentifier],
		[edgeDefaultToolbarItem itemIdentifier],
		nil];
}

- (NSCell *)tableView:(NSTableView *)tableView dataCellForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
	if ([[tableColumn identifier] isEqualToString:@"value"]) {
		/* use the row's schema's cell */
		NSCell *cell = [[[_allSchemas objectForKey:[componentToolbar selectedItemIdentifier]] objectAtIndex:row] cell];
		[cell setEnabled:[_allAttributes count] > 0];
		return cell;
	}
	else
		/* use the default cell (usually a text field) for other columns */
		return nil;
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	int selectedRow = [[aNotification object] selectedRow];
	NSString* documentation = selectedRow == -1 ? nil : [[[_allSchemas objectForKey:[componentToolbar selectedItemIdentifier]] objectAtIndex: selectedRow] documentation];
	[[documentationWeb mainFrame] loadHTMLString:documentation baseURL:[NSURL URLWithString:@"http://www.graphviz.org/"]];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
	return [[_allSchemas objectForKey:[componentToolbar selectedItemIdentifier]] count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)rowIndex
{
	NSString *selectedComponentIdentifier = [componentToolbar selectedItemIdentifier];
	NSString *attributeName = [[[_allSchemas objectForKey:selectedComponentIdentifier] objectAtIndex:rowIndex] name];
	if ([[tableColumn identifier] isEqualToString:@"key"])
		return attributeName;
	else if ([[tableColumn identifier] isEqualToString:@"value"])
		/* return the inspected graph's attribute value, if any */
		return [[_allAttributes objectForKey:selectedComponentIdentifier] valueForKey:attributeName];
	else
		return nil;
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)rowIndex
{
	if ([[tableColumn identifier] isEqualToString:@"value"])
		{
			NSString *selectedComponentIdentifier = [componentToolbar selectedItemIdentifier];
			NSString *attributeName = [[[_allSchemas objectForKey:selectedComponentIdentifier] objectAtIndex:rowIndex] name];
			
			/* set or remove the key-value on the selected attributes */
			/* NOTE: to avoid needlessly reloading the table in graphDocumentDidChange:, we fence this change with _otherChangedGraph = NO */
			_otherChangedGraph = NO;
			@try {
				[[_allAttributes objectForKey:selectedComponentIdentifier] setValue:anObject forKey:attributeName];
			}
			@finally {
				_otherChangedGraph = YES;
			}
		}

}


- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowDidBecomeMainNotification object:nil];
	[_allSchemas release];
	[_allAttributes release];
	[super dealloc];
}

@end
