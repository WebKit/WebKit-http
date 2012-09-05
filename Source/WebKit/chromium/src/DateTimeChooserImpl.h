/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef DateTimeChooserImpl_h
#define DateTimeChooserImpl_h

#include "DateTimeChooser.h"
#include "PagePopupClient.h"

#if ENABLE(CALENDAR_PICKER)

namespace WebCore {
class PagePopup;
class DateTimeChooserClient;
}

namespace WebKit {

class ChromeClientImpl;

class DateTimeChooserImpl : public WebCore::DateTimeChooser, public WebCore::PagePopupClient {
public:
    DateTimeChooserImpl(ChromeClientImpl*, WebCore::DateTimeChooserClient*, const WebCore::DateTimeChooserParameters&);
    virtual ~DateTimeChooserImpl();

    // DateTimeChooser functions:
    virtual void endChooser() OVERRIDE;

private:
    // PagePopupClient functions:
    virtual WebCore::IntSize contentSize() OVERRIDE;
    virtual void writeDocument(WebCore::DocumentWriter&) OVERRIDE;
    virtual void setValueAndClosePopup(int, const String&) OVERRIDE;
    virtual void didClosePopup() OVERRIDE;

    ChromeClientImpl* m_chromeClient;
    WebCore::DateTimeChooserClient* m_client;
    WebCore::PagePopup* m_popup;
    WebCore::DateTimeChooserParameters m_parameters;
};

}

#endif // ENABLE(CALENDAR_PICKER)

#endif // DateTimeChooserImpl_h
