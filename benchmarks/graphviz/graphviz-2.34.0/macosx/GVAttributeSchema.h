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


@interface GVAttributeSchema : NSObject {
	NSXMLElement *_element;
}

@property(readonly) NSString *name;
@property(readonly) NSCell *cell;
@property(readonly) NSString *documentation;

+ (NSArray*)attributeSchemasWithComponent:(NSString *)component;
- (id)initWithXMLElement:(NSXMLElement *)element;

- (void)dealloc;
@end
