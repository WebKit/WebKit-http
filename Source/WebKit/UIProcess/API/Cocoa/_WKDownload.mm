/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#import "config.h"
#import "_WKDownloadInternal.h"

#import "DownloadProxy.h"
#import "WKFrameInfoInternal.h"
#import "WKNSData.h"
#import "WKWebViewInternal.h"
#import <wtf/WeakObjCPtr.h>

@implementation _WKDownload {
    API::ObjectStorage<WebKit::DownloadProxy> _download;
}

- (void)dealloc
{
    _download->~DownloadProxy();

    [super dealloc];
}

- (void)cancel
{
    _download->cancel();
}

- (void)publishProgressAtURL:(NSURL *)URL
{
    _download->publishProgress(URL);
}

- (NSURLRequest *)request
{
    return _download->request().nsURLRequest(WebCore::HTTPBodyUpdatePolicy::DoNotUpdateHTTPBody);
}

- (WKWebView *)originatingWebView
{
    if (auto* originatingPage = _download->originatingPage())
        return [[fromWebPageProxy(*originatingPage) retain] autorelease];
    return nil;
}

-(NSArray<NSURL *> *)redirectChain
{
    return createNSArray(_download->redirectChain(), [] (auto& url) -> NSURL * {
        return url;
    }).autorelease();
}

- (BOOL)wasUserInitiated
{
    return _download->wasUserInitiated();
}

- (NSData *)resumeData
{
    return WebKit::wrapper(_download->resumeData());
}

- (WKFrameInfo *)originatingFrame
{
    return WebKit::wrapper(&_download->frameInfo());
}

- (id)copyWithZone:(NSZone *)zone
{
    return [self retain];
}

#pragma mark WKObject protocol implementation

- (API::Object&)_apiObject
{
    return *_download;
}

@end
