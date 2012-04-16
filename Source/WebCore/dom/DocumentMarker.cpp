/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DocumentMarker.h"

namespace WebCore {

DocumentMarkerDetails::~DocumentMarkerDetails()
{
}

class DocumentMarkerDescription : public DocumentMarkerDetails {
public:
    static PassRefPtr<DocumentMarkerDescription> create(const String&);

    const String& description() const { return m_description; }
    virtual bool isDescription() const { return true; }

private:
    DocumentMarkerDescription(const String& description)
        : m_description(description)
    {
    }

    String m_description;
};

PassRefPtr<DocumentMarkerDescription> DocumentMarkerDescription::create(const String& description)
{
    return adoptRef(new DocumentMarkerDescription(description));
}

inline DocumentMarkerDescription* toDocumentMarkerDescription(DocumentMarkerDetails* details)
{
    if (details && details->isDescription())
        return static_cast<DocumentMarkerDescription*>(details);
    return 0;
}


class DocumentMarkerTextMatch : public DocumentMarkerDetails {
public:
    static PassRefPtr<DocumentMarkerTextMatch> instanceFor(bool);

    bool activeMatch() const { return m_match; }
    virtual bool isTextMatch() const { return true; }

private:
    explicit DocumentMarkerTextMatch(bool match)
        : m_match(match)
    {
    }

    bool m_match;
};

PassRefPtr<DocumentMarkerTextMatch> DocumentMarkerTextMatch::instanceFor(bool match)
{
    DEFINE_STATIC_LOCAL(RefPtr<DocumentMarkerTextMatch>, trueInstance, (adoptRef(new DocumentMarkerTextMatch(true))));
    DEFINE_STATIC_LOCAL(RefPtr<DocumentMarkerTextMatch>, falseInstance, (adoptRef(new DocumentMarkerTextMatch(false))));
    return match ? trueInstance : falseInstance;
}

inline DocumentMarkerTextMatch* toDocumentMarkerTextMatch(DocumentMarkerDetails* details)
{
    if (details && details->isTextMatch())
        return static_cast<DocumentMarkerTextMatch*>(details);
    return 0;
}


DocumentMarker::DocumentMarker() 
    : m_type(Spelling), m_startOffset(0), m_endOffset(0)
{
}

DocumentMarker::DocumentMarker(MarkerType type, unsigned startOffset, unsigned endOffset)
    : m_type(type), m_startOffset(startOffset), m_endOffset(endOffset)
{
}

DocumentMarker::DocumentMarker(MarkerType type, unsigned startOffset, unsigned endOffset, const String& description)
    : m_type(type)
    , m_startOffset(startOffset)
    , m_endOffset(endOffset)
    , m_details(description.isEmpty() ? 0 : DocumentMarkerDescription::create(description))
{
}

DocumentMarker::DocumentMarker(unsigned startOffset, unsigned endOffset, bool activeMatch)
    : m_type(DocumentMarker::TextMatch)
    , m_startOffset(startOffset)
    , m_endOffset(endOffset)
    , m_details(DocumentMarkerTextMatch::instanceFor(activeMatch))
{
}

DocumentMarker::DocumentMarker(MarkerType type, unsigned startOffset, unsigned endOffset, PassRefPtr<DocumentMarkerDetails> details)
    : m_type(type)
    , m_startOffset(startOffset)
    , m_endOffset(endOffset)
    , m_details(details)
{
}

void DocumentMarker::shiftOffsets(int delta)
{
    m_startOffset += delta;
    m_endOffset +=  delta;
}

void DocumentMarker::setActiveMatch(bool active)
{
    m_details = DocumentMarkerTextMatch::instanceFor(active);
}

const String& DocumentMarker::description() const
{
    if (DocumentMarkerDescription* details = toDocumentMarkerDescription(m_details.get()))
        return details->description();
    return emptyString();
}

bool DocumentMarker::activeMatch() const
{
    if (DocumentMarkerTextMatch* details = toDocumentMarkerTextMatch(m_details.get()))
        return details->activeMatch();
    return false;
}

} // namespace WebCore
