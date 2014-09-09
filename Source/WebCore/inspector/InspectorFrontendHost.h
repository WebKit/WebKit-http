/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#ifndef InspectorFrontendHost_h
#define InspectorFrontendHost_h

#include "ContextMenu.h"
#include "ContextMenuProvider.h"
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ContextMenuItem;
class Event;
class FrontendMenuProvider;
class InspectorFrontendClient;
class Page;

class InspectorFrontendHost : public RefCounted<InspectorFrontendHost> {
public:
    static PassRefPtr<InspectorFrontendHost> create(InspectorFrontendClient* client, Page* frontendPage)
    {
        return adoptRef(new InspectorFrontendHost(client, frontendPage));
    }

    ~InspectorFrontendHost();
    void disconnectClient();

    void loaded();
    void requestSetDockSide(const String&);
    void closeWindow();
    void bringToFront();
    void setZoomFactor(float);
    void inspectedURLChanged(const String&);

    void setAttachedWindowHeight(unsigned);
    void setAttachedWindowWidth(unsigned);
    void setToolbarHeight(unsigned);

    void moveWindowBy(float x, float y) const;

    String localizedStringsURL();
    String debuggableType();

    void copyText(const String& text);
    void openInNewTab(const String& url);
    bool canSave();
    void save(const String& url, const String& content, bool base64Encoded, bool forceSaveAs);
    void append(const String& url, const String& content);
    void close(const String& url);

#if ENABLE(CONTEXT_MENUS)
    // Called from [Custom] implementations.
    void showContextMenu(Event*, const Vector<ContextMenuItem>& items);
#endif
    void sendMessageToBackend(const String& message);
    void dispatchEventAsContextMenuEvent(Event*);

    bool isUnderTest();

    void beep();

    bool canInspectWorkers();
    bool canSaveAs();

private:
#if ENABLE(CONTEXT_MENUS)
    friend class FrontendMenuProvider;
#endif
    InspectorFrontendHost(InspectorFrontendClient*, Page* frontendPage);

    InspectorFrontendClient* m_client;
    Page* m_frontendPage;
#if ENABLE(CONTEXT_MENUS)
    FrontendMenuProvider* m_menuProvider;
#endif
};

} // namespace WebCore

#endif // !defined(InspectorFrontendHost_h)
