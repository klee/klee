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

#import <Foundation/Foundation.h>

@class GVGraph;

@interface GVGraphArguments : NSMutableDictionary {
	GVGraph *_graph;
	NSMutableDictionary *_arguments;
}

- (id)initWithGraph:(GVGraph *)graph;

/* dictionary primitive methods */
- (NSUInteger)count;
- (NSEnumerator *)keyEnumerator;
- (id)objectForKey:(id)aKey;

/* mutable dictionary primitive methods */
- (void)setObject:(id)anObject forKey:(id)aKey;
- (void)removeObjectForKey:(id)aKey;

- (void)dealloc;

@end
