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

#import "GVGraphArguments.h"
#import "GVGraph.h"

@implementation GVGraphArguments

- (id)initWithGraph:(GVGraph *)graph
{
	if (self = [super init]) {
		_graph = graph;	/* no retain to avoid a retain cycle */
		_arguments = [[NSMutableDictionary alloc] init];
	}
	return self;
}

- (NSUInteger)count
{
	return [_arguments count];
}

- (NSEnumerator *)keyEnumerator
{
	return [_arguments keyEnumerator];
}

- (id)objectForKey:(id)aKey
{
	return [_arguments objectForKey:aKey];
}

/* mutable dictionary primitive methods */
- (void)setObject:(id)anObject forKey:(id)aKey
{
	[_arguments setObject:anObject forKey:aKey];
	[_graph noteChanged:YES];
}

- (void)removeObjectForKey:(id)aKey
{
	[_arguments removeObjectForKey:aKey];
	[_graph noteChanged:YES];
}

- (void)dealloc
{
	[_arguments release];
	[super dealloc];
}

@end
