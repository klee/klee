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

#include "gvc.h"

@class GVGraphArguments;
@class GVGraphDefaultAttributes;

@interface GVGraph : NSObject
{
@public
	graph_t *_graph;
@protected
	BOOL _freeLastLayout;
	
	GVGraphArguments *_arguments;
	GVGraphDefaultAttributes *_graphAttributes;
	GVGraphDefaultAttributes *_defaultNodeAttributes;
	GVGraphDefaultAttributes *_defaultEdgeAttributes;
}

@property(readonly) graph_t *graph;
@property(readonly) GVGraphArguments *arguments;
@property(readonly) GVGraphDefaultAttributes *graphAttributes;
@property(readonly) GVGraphDefaultAttributes *defaultNodeAttributes;
@property(readonly) GVGraphDefaultAttributes *defaultEdgeAttributes;

+ (void)initialize;
+ (NSArray *)pluginsWithAPI:(api_t)api;

- (id)initWithURL:(NSURL *)URL error:(NSError **)outError;

- (NSData *)renderWithFormat:(NSString *)format;
- (void)renderWithFormat:(NSString*)format toURL:(NSURL *)URL;
- (void)noteChanged:(BOOL)relayout;

- (BOOL)writeToURL:(NSURL *)URL error:(NSError **)outError;

- (void)dealloc;

@end

extern NSString *const GVGraphvizErrorDomain;

enum {
	GVNoError,
	GVFileParseError
};
