/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef DRTDevToolsClient_h
#define DRTDevToolsClient_h

#include "Task.h"
#include "WebDevToolsFrontendClient.h"
#include "platform/WebString.h"
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
namespace WebKit {

class WebDevToolsFrontend;
struct WebDevToolsMessageData;
class WebView;

} // namespace WebKit

class DRTDevToolsAgent;

class DRTDevToolsClient : public WebKit::WebDevToolsFrontendClient {
    WTF_MAKE_NONCOPYABLE(DRTDevToolsClient);
public:
    DRTDevToolsClient(DRTDevToolsAgent*, WebKit::WebView*);
    virtual ~DRTDevToolsClient();
    void reset();

    // WebDevToolsFrontendClient implementation
    virtual void sendMessageToBackend(const WebKit::WebString&);

    virtual void activateWindow();
    virtual void closeWindow();
    virtual void dockWindow();
    virtual void undockWindow();

    void asyncCall(const WebKit::WebString& args);

    void allMessagesProcessed();
    TaskList* taskList() { return &m_taskList; }

 private:
    void call(const WebKit::WebString& args);
    class AsyncCallTask: public MethodTask<DRTDevToolsClient> {
    public:
        AsyncCallTask(DRTDevToolsClient* object, const WebKit::WebString& args)
            : MethodTask<DRTDevToolsClient>(object), m_args(args) { }
        virtual void runIfValid() { m_object->call(m_args); }

    private:
        WebKit::WebString m_args;
    };

    TaskList m_taskList;
    WebKit::WebView* m_webView;
    DRTDevToolsAgent* m_drtDevToolsAgent;
    WTF::OwnPtr<WebKit::WebDevToolsFrontend> m_webDevToolsFrontend;
};

#endif // DRTDevToolsClient_h
