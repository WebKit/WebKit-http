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

#include "config.h"
#include "WKDownload.h"

#include "APIArray.h"
#include "APIData.h"
#include "APIDownloadClient.h"
#include "APIURLRequest.h"
#include "DownloadProxy.h"
#include "WKAPICast.h"
#include "WebPageProxy.h"

using namespace WebKit;

WKTypeID WKDownloadGetTypeID()
{
    return toAPI(DownloadProxy::APIType);
}

uint64_t WKDownloadGetID(WKDownloadRef)
{
    return 0;
}

WKURLRequestRef WKDownloadCopyRequest(WKDownloadRef download)
{
    return toAPI(&API::URLRequest::create(toImpl(download)->request()).leakRef());
}

WKDataRef WKDownloadGetResumeData(WKDownloadRef download)
{
    return toAPI(toImpl(download)->legacyResumeData());
}

void WKDownloadCancel(WKDownloadRef download)
{
    return toImpl(download)->cancel([download = makeRef(*toImpl(download))] (auto*) {
        download->client().legacyDidCancel(download.get());
    });
}

WKPageRef WKDownloadGetOriginatingPage(WKDownloadRef download)
{
    return toAPI(toImpl(download)->originatingPage());
}

WKArrayRef WKDownloadCopyRedirectChain(WKDownloadRef download)
{
    auto& redirectChain =  toImpl(download)->redirectChain();
    Vector<RefPtr<API::Object>> urls;
    urls.reserveInitialCapacity(redirectChain.size());
    for (auto& redirectURL : redirectChain)
        urls.uncheckedAppend(API::URL::create(redirectURL.string()));
    return toAPI(&API::Array::create(WTFMove(urls)).leakRef());
}

bool WKDownloadGetWasUserInitiated(WKDownloadRef download)
{
    return toImpl(download)->wasUserInitiated();
}
