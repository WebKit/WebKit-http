/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleHitTestResult.h>
#include <WebKit2/WKBundleInitialize.h>
#include <WebKit2/WKBundlePage.h>
#include <glib.h>
#include <string.h>

static WKBundleRef globalBundle;

static void mouseDidMoveOverElement(WKBundlePageRef page, WKBundleHitTestResultRef hitTestResult, WKEventModifiers modifiers, WKTypeRef *userData, const void *clientInfo)
{
    *userData = WKBundleHitTestResultCopyAbsoluteLinkURL(hitTestResult);
}

static void didCreatePage(WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo)
{
    WKBundlePageUIClient uiClient;
    memset(&uiClient, 0, sizeof(uiClient));
    uiClient.version = kWKBundlePageUIClientCurrentVersion;
    uiClient.mouseDidMoveOverElement = mouseDidMoveOverElement;

    WKBundlePageSetUIClient(page, &uiClient);
}

void WKBundleInitialize(WKBundleRef bundle, WKTypeRef initializationUserData)
{
    globalBundle = bundle;

    WKBundleClient client = {
        kWKBundleClientCurrentVersion,
        0,
        didCreatePage,
        0, /* willDestroyPage */
        0, /* didInitializePageGroup */
        0, /* didRecieveMessage */
    };
    WKBundleSetClient(bundle, &client);
}
