/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "MiniBrowserWebProcessPlugIn.h"

#if WK_API_ENABLED

#import "MockContentFilterEnabler.h"

@implementation MiniBrowserWebProcessPlugIn {
    WebMockContentFilterEnabler *_contentFilterEnabler;
    WKWebProcessPlugInController *_plugInController;
}

- (void)webProcessPlugIn:(WKWebProcessPlugInController *)plugInController initializeWithObject:(id)initializationObject
{
    _plugInController = [plugInController retain];
    [_plugInController.parameters addObserver:self forKeyPath:NSStringFromClass([WebMockContentFilterEnabler class]) options:NSKeyValueObservingOptionInitial context:NULL];
}

- (void)dealloc
{
    [_plugInController.parameters removeObserver:self forKeyPath:NSStringFromClass([WebMockContentFilterEnabler class])];
    [_plugInController release];
    [_contentFilterEnabler release];
    [super dealloc];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    WebMockContentFilterEnabler *enabler = [[object valueForKeyPath:keyPath] retain];
    [_contentFilterEnabler release];
    _contentFilterEnabler = enabler;
    [_contentFilterEnabler enable];
}

@end

#endif // WK_API_ENABLED
