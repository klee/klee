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

#include <fcntl.h>
#include <sys/event.h>

#import "GVFileNotificationCenter.h"

static GVFileNotificationCenter *_defaultCenter = nil;

@interface GVFileNotificationRecord : NSObject
{
	id _observer;
	NSString *_path;
	SEL _selector;
	int _fileDescriptor;
}

@property(readonly) id observer;
@property(readonly) NSString *path;
@property SEL selector;
@property int fileDescriptor;

@end

@implementation GVFileNotificationRecord

@synthesize observer = _observer;
@synthesize path = _path;
@synthesize selector = _selector;
@synthesize fileDescriptor = _fileDescriptor;

- (id)initWithObserver:(id)observer path:(NSString *)path
{
	if (self = [super init]) {
		_observer = observer;
		_path = [path retain];
	}
	return self;
}

- (BOOL)isEqual:(id)anObject
{
	/* if the other object is one of us, compare observers + paths only */
	return [anObject isKindOfClass:[GVFileNotificationRecord class]] ? [self observer] == [anObject observer] && [[self path] isEqualToString:[anObject path]] : NO;
}

- (NSUInteger)hash
{
	/* hash based on observers + paths only */
	return (NSUInteger)_observer ^ [_path hash];
}

- (void)dealloc
{
	[_path release];
	[super dealloc];
}

@end

static void noteFileChanged(CFFileDescriptorRef queue, CFOptionFlags callBackTypes, void *info)
{
	int queueDescriptor = CFFileDescriptorGetNativeDescriptor(queue);
    
	/* grab the next event from the kernel queue */
	struct kevent event;
    kevent(queueDescriptor, NULL, 0, &event, 1, NULL);
	
	GVFileNotificationRecord *record = (GVFileNotificationRecord *)event.udata;
	if (record) {
		/* report the file change to the observer */
		[record.observer performSelector:record.selector withObject:record.path];

		/* if watched file is deleted, try to reopen the file and watch it again */
		if (event.fflags & NOTE_DELETE) {
			close(record.fileDescriptor);
			
			int fileDescriptor = open([record.path UTF8String], O_EVTONLY);
			if (fileDescriptor != -1) {
				struct kevent change;
				EV_SET (&change, fileDescriptor, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND, 0, record);
				if (kevent (queueDescriptor, &change, 1, NULL, 0, NULL) != -1)
					record.fileDescriptor = fileDescriptor;
			}
		}
		
	}
	
	/* reenable the callbacks (they were automatically disabled when handling the CFFileDescriptor) */
    CFFileDescriptorEnableCallBacks(queue, kCFFileDescriptorReadCallBack);
}


@implementation GVFileNotificationCenter

+ (void)initialize
{
	if (!_defaultCenter)
		_defaultCenter = [[GVFileNotificationCenter alloc] init];
}

+ (id)defaultCenter
{
	return _defaultCenter;
}

- (id)init
{
	if (self = [super init]) {
		/* create kernel queue, wrap a CFFileDescriptor around it and schedule it on the Cocoa run loop */
		_queue = CFFileDescriptorCreate(kCFAllocatorDefault, kqueue(), true, noteFileChanged, NULL);
		CFFileDescriptorEnableCallBacks(_queue, kCFFileDescriptorReadCallBack);
		CFRunLoopAddSource(
			[[NSRunLoop currentRunLoop] getCFRunLoop],
			CFFileDescriptorCreateRunLoopSource(kCFAllocatorDefault, _queue, 0),
			kCFRunLoopDefaultMode);
			
		/* need to keep track of observers */
		_records = [[NSMutableSet alloc] init];
	}
	return self;
}

- (void)addObserver:(id)observer selector:(SEL)selector path:(NSString *)path
{
	GVFileNotificationRecord *record = [[[GVFileNotificationRecord alloc] initWithObserver:observer path:path] autorelease];
	GVFileNotificationRecord *oldRecord = [_records member:record];
	
	if (oldRecord)
		/* record already exists, just update the selector */
		oldRecord.selector = selector;
	else {
		/* new record, start monitoring the path */
		int fileDescriptor = open([path UTF8String], O_EVTONLY);
		if (fileDescriptor != -1) {
			struct kevent change;
			EV_SET (&change, fileDescriptor, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND, 0, record);
			if (kevent (CFFileDescriptorGetNativeDescriptor(_queue), &change, 1, NULL, 0, NULL) != -1) {
				record.selector = selector;
				record.fileDescriptor = fileDescriptor;
				[_records addObject:record];
			}
		}
	}
}

- (void)removeObserver:(id)observer path:(NSString *)path
{
	GVFileNotificationRecord *record = [_records member:[[[GVFileNotificationRecord alloc] initWithObserver:observer path:path] autorelease]];
	if (record) {
		close(record.fileDescriptor);	/* closing the file descriptor also removes it from the kqueue */
		[_records removeObject:record];
	}
	
}
@end
