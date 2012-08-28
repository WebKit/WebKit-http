/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebKitDLL.h"
#include "WebKitStatistics.h"

#include "WebKitStatisticsPrivate.h"
#include <WebCore/BString.h>

using namespace WebCore;

int WebViewCount;
int WebDataSourceCount;
int WebFrameCount;
int WebHTMLRepresentationCount;
int WebFrameViewCount;

// WebKitStatistics ---------------------------------------------------------------------------

WebKitStatistics::WebKitStatistics()
: m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebKitStatistics");
}

WebKitStatistics::~WebKitStatistics()
{
    gClassCount--;
    gClassNameCount.remove("WebKitStatistics");
}

WebKitStatistics* WebKitStatistics::createInstance()
{
    WebKitStatistics* instance = new WebKitStatistics();
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebKitStatistics::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<WebKitStatistics*>(this);
    else if (IsEqualGUID(riid, IID_IWebKitStatistics))
        *ppvObject = static_cast<WebKitStatistics*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebKitStatistics::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebKitStatistics::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebKitStatistics ------------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebKitStatistics::webViewCount( 
    /* [retval][out] */ int *count)
{
    *count = WebViewCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebKitStatistics::frameCount( 
    /* [retval][out] */ int *count)
{
    *count = WebFrameCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebKitStatistics::dataSourceCount( 
    /* [retval][out] */ int *count)
{
    *count = WebDataSourceCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebKitStatistics::viewCount( 
    /* [retval][out] */ int *count)
{
    *count = WebFrameViewCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebKitStatistics::HTMLRepresentationCount( 
    /* [retval][out] */ int *count)
{
    *count = WebHTMLRepresentationCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebKitStatistics::comClassCount( 
    /* [retval][out] */ int *classCount)
{
    *classCount = gClassCount;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebKitStatistics::comClassNameCounts( 
    /* [retval][out] */ BSTR *output)
{
    typedef HashCountedSet<String>::const_iterator Iterator;
    Iterator end = gClassNameCount.end();
    Vector<UChar> vector;
    for (Iterator current = gClassNameCount.begin(); current != end; ++current) {
        append(vector, String::format("%4u", current->second));
        vector.append('\t');
        append(vector, static_cast<String>(current->first));
        vector.append('\n');
    }
    *output = BString(String::adopt(vector)).release();
    return S_OK;
}
