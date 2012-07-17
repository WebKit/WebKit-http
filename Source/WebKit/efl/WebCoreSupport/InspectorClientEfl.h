/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2012 Samsung Electronics
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

#ifndef InspectorClientEfl_h
#define InspectorClientEfl_h

#if ENABLE(INSPECTOR)

#include "InspectorClient.h"
#include "InspectorFrontendChannel.h"
#include "InspectorFrontendClientLocal.h"
#include <Evas.h>
#include <wtf/Forward.h>

namespace WebCore {
class InspectorFrontendClientEfl;
class Page;

class InspectorClientEfl : public InspectorClient, public InspectorFrontendChannel {
public:
    explicit InspectorClientEfl(Evas_Object*);
    ~InspectorClientEfl();

    virtual void inspectorDestroyed();

    virtual InspectorFrontendChannel* openInspectorFrontend(InspectorController*);
    virtual void closeInspectorFrontend();
    virtual void bringFrontendToFront();

    virtual void highlight();
    virtual void hideHighlight();

    virtual bool sendMessageToFrontend(const String&);

    void releaseFrontendPage();
    String inspectorFilesPath();

private:
    Evas_Object* m_inspectedView;
    Evas_Object* m_inspectorView;
    InspectorFrontendClientEfl* m_frontendClient;
};

class InspectorFrontendClientEfl : public InspectorFrontendClientLocal {
public:
    InspectorFrontendClientEfl(Evas_Object*, Evas_Object*, InspectorClientEfl*);
    ~InspectorFrontendClientEfl();

    virtual String localizedStringsURL();
    virtual String hiddenPanels();

    virtual void bringToFront();
    virtual void closeWindow();

    virtual void inspectedURLChanged(const String&);

    virtual void attachWindow();
    virtual void detachWindow();

    virtual void setAttachedWindowHeight(unsigned);

    void disconnectInspectorClient() { m_inspectorClient = 0; }
    void destroyInspectorWindow(bool notifyInspectorController);

private:
    Evas_Object* m_inspectedView;
    Evas_Object* m_inspectorView;
    InspectorClientEfl* m_inspectorClient;

};
}

#endif
#endif // InspectorClientEfl_h
