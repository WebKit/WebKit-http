/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "CachedScript.h"

#include "MemoryCache.h"
#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"
#include "WebCoreMemoryInstrumentation.h"
#include <wtf/Vector.h>

#if USE(JSC)  
#include <parser/SourceProvider.h>
#endif

namespace WebCore {

CachedScript::CachedScript(const ResourceRequest& resourceRequest, const String& charset)
    : CachedResource(resourceRequest, Script)
    , m_decoder(TextResourceDecoder::create("application/javascript", charset))
{
    // It's javascript we want.
    // But some websites think their scripts are <some wrong mimetype here>
    // and refuse to serve them if we only accept application/x-javascript.
    setAccept("*/*");
}

CachedScript::~CachedScript()
{
}

void CachedScript::setEncoding(const String& chs)
{
    m_decoder->setEncoding(chs, TextResourceDecoder::EncodingFromHTTPHeader);
}

String CachedScript::encoding() const
{
    return m_decoder->encoding().name();
}

const String& CachedScript::script()
{
    ASSERT(!isPurgeable());

    if (!m_script && m_data) {
        m_script = m_decoder->decode(m_data->data(), encodedSize());
        m_script.append(m_decoder->flush());
        setDecodedSize(m_script.sizeInBytes());
    }
    m_decodedDataDeletionTimer.startOneShot(0);
    
    return m_script;
}

void CachedScript::data(PassRefPtr<SharedBuffer> data, bool allDataReceived)
{
    if (!allDataReceived)
        return;

    m_data = data;
    setEncodedSize(m_data.get() ? m_data->size() : 0);
    setLoading(false);
    checkNotify();
}

void CachedScript::destroyDecodedData()
{
    m_script = String();
    unsigned extraSize = 0;
#if USE(JSC)
    if (m_sourceProviderCache && m_clients.isEmpty())
        m_sourceProviderCache->clear();

    extraSize = m_sourceProviderCache ? m_sourceProviderCache->byteSize() : 0;
#endif
    setDecodedSize(extraSize);
    if (!MemoryCache::shouldMakeResourcePurgeableOnEviction() && isSafeToMakePurgeable())
        makePurgeable(true);
}

#if USE(JSC)
JSC::SourceProviderCache* CachedScript::sourceProviderCache() const
{   
    if (!m_sourceProviderCache) 
        m_sourceProviderCache = adoptPtr(new JSC::SourceProviderCache); 
    return m_sourceProviderCache.get(); 
}

void CachedScript::sourceProviderCacheSizeChanged(int delta)
{
    setDecodedSize(decodedSize() + delta);
}
#endif

void CachedScript::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CachedResourceScript);
    CachedResource::reportMemoryUsage(memoryObjectInfo);
    info.addMember(m_script);
    info.addMember(m_decoder);
#if USE(JSC)
    info.addMember(m_sourceProviderCache);
#endif
}

} // namespace WebCore
