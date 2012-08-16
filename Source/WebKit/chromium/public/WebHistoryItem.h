/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef WebHistoryItem_h
#define WebHistoryItem_h

#include "platform/WebCommon.h"
#include "platform/WebPrivatePtr.h"

namespace WebCore { class HistoryItem; }

namespace WebKit {
class WebHTTPBody;
class WebString;
class WebSerializedScriptValue;
struct WebPoint;
template <typename T> class WebVector;

// Represents a frame-level navigation entry in session history.  A
// WebHistoryItem is a node in a tree.
//
// Copying a WebHistoryItem is cheap.
//
class WebHistoryItem {
public:
    ~WebHistoryItem() { reset(); }

    WebHistoryItem() { }
    WebHistoryItem(const WebHistoryItem& h) { assign(h); }
    WebHistoryItem& operator=(const WebHistoryItem& h)
    {
        assign(h);
        return *this;
    }

    WEBKIT_EXPORT void initialize();
    WEBKIT_EXPORT void reset();
    WEBKIT_EXPORT void assign(const WebHistoryItem&);

    bool isNull() const { return m_private.isNull(); }

    WEBKIT_EXPORT WebString urlString() const;
    WEBKIT_EXPORT void setURLString(const WebString&);

    WEBKIT_EXPORT WebString originalURLString() const;
    WEBKIT_EXPORT void setOriginalURLString(const WebString&);

    WEBKIT_EXPORT WebString referrer() const;
    WEBKIT_EXPORT void setReferrer(const WebString&);

    WEBKIT_EXPORT WebString target() const;
    WEBKIT_EXPORT void setTarget(const WebString&);

    WEBKIT_EXPORT WebString parent() const;
    WEBKIT_EXPORT void setParent(const WebString&);

    WEBKIT_EXPORT WebString title() const;
    WEBKIT_EXPORT void setTitle(const WebString&);

    WEBKIT_EXPORT WebString alternateTitle() const;
    WEBKIT_EXPORT void setAlternateTitle(const WebString&);

    WEBKIT_EXPORT double lastVisitedTime() const;
    WEBKIT_EXPORT void setLastVisitedTime(double);

    WEBKIT_EXPORT WebPoint scrollOffset() const;
    WEBKIT_EXPORT void setScrollOffset(const WebPoint&);

    WEBKIT_EXPORT float pageScaleFactor() const;
    WEBKIT_EXPORT void setPageScaleFactor(float);

    WEBKIT_EXPORT bool isTargetItem() const;
    WEBKIT_EXPORT void setIsTargetItem(bool);

    WEBKIT_EXPORT int visitCount() const;
    WEBKIT_EXPORT void setVisitCount(int);

    WEBKIT_EXPORT WebVector<WebString> documentState() const;
    WEBKIT_EXPORT void setDocumentState(const WebVector<WebString>&);

    WEBKIT_EXPORT long long itemSequenceNumber() const;
    WEBKIT_EXPORT void setItemSequenceNumber(long long);

    WEBKIT_EXPORT long long documentSequenceNumber() const;
    WEBKIT_EXPORT void setDocumentSequenceNumber(long long);

    WEBKIT_EXPORT WebSerializedScriptValue stateObject() const;
    WEBKIT_EXPORT void setStateObject(const WebSerializedScriptValue&);

    WEBKIT_EXPORT WebString httpContentType() const;
    WEBKIT_EXPORT void setHTTPContentType(const WebString&);

    WEBKIT_EXPORT WebHTTPBody httpBody() const;
    WEBKIT_EXPORT void setHTTPBody(const WebHTTPBody&);

    WEBKIT_EXPORT WebVector<WebHistoryItem> children() const;
    WEBKIT_EXPORT void setChildren(const WebVector<WebHistoryItem>&);
    WEBKIT_EXPORT void appendToChildren(const WebHistoryItem&);

    WEBKIT_EXPORT WebVector<WebString> getReferencedFilePaths() const;

#if WEBKIT_IMPLEMENTATION
    WebHistoryItem(const WTF::PassRefPtr<WebCore::HistoryItem>&);
    WebHistoryItem& operator=(const WTF::PassRefPtr<WebCore::HistoryItem>&);
    operator WTF::PassRefPtr<WebCore::HistoryItem>() const;
#endif

private:
    void ensureMutable();
    WebPrivatePtr<WebCore::HistoryItem> m_private;
};

} // namespace WebKit

#endif
