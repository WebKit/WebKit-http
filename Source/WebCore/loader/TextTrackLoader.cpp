/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "TextTrackLoader.h"

#include "CachedResourceLoader.h"
#include "CachedTextTrack.h"
#include "Document.h"
#include "Logging.h"
#include "ResourceHandle.h"
#include "SharedBuffer.h"
#include "WebVTTParser.h"

namespace WebCore {
    
TextTrackLoader::TextTrackLoader(TextTrackLoaderClient* client, ScriptExecutionContext* context)
    : m_client(client)
    , m_scriptExecutionContext(context)
    , m_cueLoadTimer(this, &TextTrackLoader::cueLoadTimerFired)
    , m_state(Idle)
    , m_parseOffset(0)
    , m_newCuesAvailable(false)
{
}

TextTrackLoader::~TextTrackLoader()
{
    if (m_cachedCueData)
        m_cachedCueData->removeClient(this);
}

void TextTrackLoader::cueLoadTimerFired(Timer<TextTrackLoader>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_cueLoadTimer);
    
    if (m_newCuesAvailable) {
        m_newCuesAvailable = false;
        m_client->newCuesAvailable(this); 
    }
    
    if (m_state >= Finished)
        m_client->cueLoadingCompleted(this, m_state == Failed);
}

void TextTrackLoader::cancelLoad()
{
    if (m_cachedCueData) {
        m_cachedCueData->removeClient(this);
        m_cachedCueData = 0;
    }
}

void TextTrackLoader::processNewCueData(CachedResource* resource)
{
    ASSERT(m_cachedCueData == resource);
    
    if (m_state == Failed || !resource->data())
        return;
    
    SharedBuffer* buffer = resource->data();
    if (m_parseOffset == buffer->size())
        return;

    const char* data;
    unsigned length;
    
    if (!m_cueParser) {
        if (resource->response().mimeType() == "text/vtt")
            m_cueParser = WebVTTParser::create(this, m_scriptExecutionContext);
        else {
            // Don't proceed until we have enough data to check for the WebVTT magic identifier.
            unsigned identifierLength = WebVTTParser::fileIdentifierMaximumLength();
            if (buffer->size() < identifierLength)
                return;
            
            Vector<char> identifier;
            unsigned offset = 0;
            while (offset < identifierLength && (length = buffer->getSomeData(data, offset))) {
                if (length > identifierLength)
                    length = identifierLength;
                identifier.append(data, length);
                offset += length;
            }
            
            if (!WebVTTParser::hasRequiredFileIdentifier(identifier.data(), identifier.size())) {
                LOG(Media, "TextTrackLoader::didReceiveData - file \"%s\" does not have WebVTT magic header", 
                    resource->response().url().string().utf8().data());
                m_state = Failed;
                m_cueLoadTimer.startOneShot(0);
                return;
            }
            
            m_cueParser = WebVTTParser::create(this, m_scriptExecutionContext);
        }
    }
    
    ASSERT(m_cueParser);
    
    while ((length = buffer->getSomeData(data, m_parseOffset))) {
        m_cueParser->parseBytes(data, length);
        m_parseOffset += length;
    }
    
}

void TextTrackLoader::didReceiveData(CachedResource* resource)
{
    ASSERT(m_cachedCueData == resource);
    
    if (!resource->data())
        return;
    
    processNewCueData(resource);
}

void TextTrackLoader::notifyFinished(CachedResource* resource)
{
    ASSERT(m_cachedCueData == resource);

    processNewCueData(resource);

    if (m_state != Failed)
        m_state = resource->errorOccurred() ? Failed : Finished;

    if (!m_cueLoadTimer.isActive())
        m_cueLoadTimer.startOneShot(0);
    
    cancelLoad();
}

bool TextTrackLoader::load(const KURL& url)
{
    if (!m_client->shouldLoadCues(this))
        return false;
    
    cancelLoad();
    
    ASSERT(m_scriptExecutionContext->isDocument());
    Document* document = static_cast<Document*>(m_scriptExecutionContext);
    
    ResourceRequest cueRequest(document->completeURL(url));
    CachedResourceLoader* cachedResourceLoader = document->cachedResourceLoader();
    m_cachedCueData = static_cast<CachedTextTrack*>(cachedResourceLoader->requestTextTrack(cueRequest));
    if (m_cachedCueData)
        m_cachedCueData->addClient(this);
    
    m_client->cueLoadingStarted(this);
    
    return true;
}

void TextTrackLoader::newCuesParsed()
{
    if (m_cueLoadTimer.isActive())
        return;

    m_newCuesAvailable = true;
    m_cueLoadTimer.startOneShot(0);
}

void TextTrackLoader::getNewCues(Vector<RefPtr<TextTrackCue> >& outputCues)
{
    ASSERT(m_cueParser);
    if (m_cueParser)
        m_cueParser->getNewCues(outputCues);
}

}

#endif
