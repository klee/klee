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


#import "GVGraph.h"
#import "GVGraphArguments.h"
#import "GVGraphDefaultAttributes.h"

NSString *const GVGraphvizErrorDomain = @"GVGraphvizErrorDomain";

extern double PSinputscale;

static GVC_t *_graphContext = nil;

@implementation GVGraph

@synthesize graph = _graph;
@synthesize arguments = _arguments;
@synthesize graphAttributes = _graphAttributes;
@synthesize defaultNodeAttributes = _defaultNodeAttributes;
@synthesize defaultEdgeAttributes = _defaultEdgeAttributes;

extern char *gvplugin_list(GVC_t * gvc, api_t api, const char *str);

+ (void)initialize
{
	_graphContext = gvContext();
}

+ (NSArray *)pluginsWithAPI:(api_t)api
{
	NSMutableSet *plugins = [NSMutableSet set];
	
	/* go through each non-empty plugin in the list, ignoring the package part */
	char *pluginList = gvplugin_list(_graphContext, api, ":");
	char *restOfPlugins;
	char *nextPlugin;
	for (restOfPlugins = pluginList; nextPlugin = strsep(&restOfPlugins, " ");) {
		if (*nextPlugin) {
			char *lastColon = strrchr(nextPlugin, ':');
			if (lastColon) {
				*lastColon = '\0';
				[plugins addObject:[NSString stringWithCString:nextPlugin encoding:NSUTF8StringEncoding]];
			}
		}
	}
	free(pluginList);

	return [[plugins allObjects] sortedArrayUsingSelector:@selector(compare:)];
}

- (id)initWithURL:(NSURL *)URL error:(NSError **)outError
{
	char *parentDir,*ptr;
	if (self = [super init]) {
		if ([URL isFileURL]) {
			/* open a FILE* on the file URL */
			FILE *file = fopen([[URL path] fileSystemRepresentation], "r");
			if (!file) {
				if (outError)
					*outError = [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil];
				[self autorelease];
				return nil;
			}
			
#ifdef WITH_CGRAPH
			_graph = agread(file,0);
#else
			_graph = agread(file);
#endif
			if (!_graph) {
				if (outError)
					*outError = [NSError errorWithDomain:GVGraphvizErrorDomain code:GVFileParseError userInfo:nil];
				[self autorelease];
				return nil;
			}
			if(!agget(_graph,"imagepath")){
					parentDir = (char *)[[URL path] fileSystemRepresentation];
					ptr = strrchr(parentDir,'/');
					*ptr = 0;
#ifdef WITH_CGRAPH
					agattr(_graph,AGRAPH,"imagepath",parentDir);
#else
					agraphattr(_graph,"imagepath",parentDir);
#endif
			}
			fclose(file);
		}
		else {
			/* read the URL into memory */
			NSMutableData *memory = [NSMutableData dataWithContentsOfURL:URL options:0 error:outError];
			if (!memory) {
				[self autorelease];
				return nil;
			}
			
			/* null terminate the data */
			char nullByte = '\0';
			[memory appendBytes:&nullByte length:1];
			
			_graph = agmemread((char*)[memory bytes]);
			if (!_graph) {
				if (outError)
					*outError = [NSError errorWithDomain:GVGraphvizErrorDomain code:GVFileParseError userInfo:nil];
				[self autorelease];
				return nil;
			}
		}

		_freeLastLayout = NO;
		_arguments = [[GVGraphArguments alloc] initWithGraph:self];
#ifdef WITH_CGRAPH
		_graphAttributes = [[GVGraphDefaultAttributes alloc] initWithGraph:self prototype:AGRAPH];
		_defaultNodeAttributes = [[GVGraphDefaultAttributes alloc] initWithGraph:self prototype:AGNODE];
		_defaultEdgeAttributes = [[GVGraphDefaultAttributes alloc] initWithGraph:self prototype:AGEDGE];
#else
		_graphAttributes = [[GVGraphDefaultAttributes alloc] initWithGraph:self prototype:_graph];
		_defaultNodeAttributes = [[GVGraphDefaultAttributes alloc] initWithGraph:self prototype:agprotonode(_graph)];
		_defaultEdgeAttributes = [[GVGraphDefaultAttributes alloc] initWithGraph:self prototype:agprotoedge(_graph)];
#endif
	}
	
	return self;
}

- (BOOL)writeToURL:(NSURL *)URL error:(NSError **)outError
{
	if ([URL isFileURL]) {
		/* open a FILE* on the file URL */
		FILE *file = fopen([[URL path] fileSystemRepresentation], "w");
		if (!file) {
			if (outError)
				*outError = [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil];
			return NO;
		}
		
		/* write it out */
		if (agwrite(_graph, file) != 0) {
			if (outError)
				*outError = [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil];
			return NO;
		}
		
		fclose(file);
		return YES;
	}
	else
		/* can't write out to non-file URL */
		return NO;
}

- (void)noteChanged:(BOOL)relayout
{
	/* if we need to layout, apply globals and then relayout */
	if (relayout) {
		NSString* layout = [_arguments objectForKey:@"layout"];
		if (layout) {
			if (_freeLastLayout)
				gvFreeLayout(_graphContext, _graph);
				
			/* apply scale */
			NSString* scale = [_arguments objectForKey:@"scale"];
			PSinputscale = scale ? [scale doubleValue] : 0.0;
			if (PSinputscale == 0.0)
				PSinputscale = 72.0;
		
			if (gvLayout(_graphContext, _graph, (char*)[layout UTF8String]) != 0)
				@throw [NSException exceptionWithName:@"GVException" reason:@"bad layout" userInfo:nil];
			_freeLastLayout = YES;
		}
	}
	

	[[NSNotificationCenter defaultCenter] postNotificationName: @"GVGraphDidChange" object:self];
}

- (NSData*)renderWithFormat:(NSString *)format
{
	char *renderedData = NULL;
	unsigned int renderedLength = 0;
	if (gvRenderData(_graphContext, _graph, (char*)[format UTF8String], &renderedData, &renderedLength) != 0)
		@throw [NSException exceptionWithName:@"GVException" reason:@"bad render" userInfo:nil];
	return [NSData dataWithBytesNoCopy:renderedData length:renderedLength freeWhenDone:YES];

}

- (void)renderWithFormat:(NSString *)format toURL:(NSURL *)URL
{
	if ([URL isFileURL]) {
		if (gvRenderFilename(_graphContext, _graph, (char*)[format UTF8String], (char*)[[URL path] UTF8String]) != 0)
			@throw [NSException exceptionWithName:@"GVException" reason:@"bad render" userInfo:nil];
	}
	else
		[[self renderWithFormat:format] writeToURL:URL atomically:NO];
}


- (void)dealloc
{
	if (_graph)
		agclose(_graph);
	
	[_arguments release];
	[_graphAttributes release];
	[_defaultNodeAttributes release];
	[_defaultEdgeAttributes release];
	[super dealloc];
}

@end
