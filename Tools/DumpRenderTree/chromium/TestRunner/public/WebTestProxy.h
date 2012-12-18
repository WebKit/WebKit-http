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

#ifndef WebTestProxy_h
#define WebTestProxy_h

#include "Platform/chromium/public/WebRect.h"
#include "WebKit/chromium/public/WebAccessibilityNotification.h"
#include "WebKit/chromium/public/WebDragOperation.h"
#include "WebKit/chromium/public/WebEditingAction.h"
#include "WebKit/chromium/public/WebNavigationPolicy.h"
#include "WebKit/chromium/public/WebTextAffinity.h"

namespace WebKit {
class WebAccessibilityObject;
class WebDragData;
class WebFrame;
class WebImage;
class WebIntentRequest;
class WebIntentServiceInfo;
class WebNode;
class WebRange;
class WebString;
struct WebPoint;
class WebSerializedScriptValue;
struct WebSize;
}

namespace WebTestRunner {

class WebTestDelegate;
class WebTestInterfaces;
class WebTestRunner;

class WebTestProxyBase {
public:
    void setInterfaces(WebTestInterfaces*);
    void setDelegate(WebTestDelegate*);

    void setPaintRect(const WebKit::WebRect&);
    WebKit::WebRect paintRect() const;

protected:
    WebTestProxyBase();
    ~WebTestProxyBase();

    void didInvalidateRect(const WebKit::WebRect&);
    void didScrollRect(int, int, const WebKit::WebRect&);
    void scheduleComposite();
    void scheduleAnimation();
    void setWindowRect(const WebKit::WebRect&);
    void show(WebKit::WebNavigationPolicy);
    void didAutoResize(const WebKit::WebSize&);
    void postAccessibilityNotification(const WebKit::WebAccessibilityObject&, WebKit::WebAccessibilityNotification);
    void startDragging(WebKit::WebFrame*, const WebKit::WebDragData&, WebKit::WebDragOperationsMask, const WebKit::WebImage&, const WebKit::WebPoint&);
    bool shouldBeginEditing(const WebKit::WebRange&);
    bool shouldEndEditing(const WebKit::WebRange&);
    bool shouldInsertNode(const WebKit::WebNode&, const WebKit::WebRange&, WebKit::WebEditingAction);
    bool shouldInsertText(const WebKit::WebString& text, const WebKit::WebRange&, WebKit::WebEditingAction);
    bool shouldChangeSelectedRange(const WebKit::WebRange& fromRange, const WebKit::WebRange& toRange, WebKit::WebTextAffinity, bool stillSelecting);
    bool shouldDeleteRange(const WebKit::WebRange&);
    bool shouldApplyStyle(const WebKit::WebString& style, const WebKit::WebRange&);
    void didBeginEditing();
    void didChangeSelection(bool isEmptySelection);
    void didChangeContents();
    void didEndEditing();
    void registerIntentService(WebKit::WebFrame*, const WebKit::WebIntentServiceInfo&);
    void dispatchIntent(WebKit::WebFrame* source, const WebKit::WebIntentRequest&);

private:
    WebTestInterfaces* m_testInterfaces;
    WebTestDelegate* m_delegate;

    WebKit::WebRect m_paintRect;
};

// Use this template to inject methods into your WebViewClient implementation
// required for the running layout tests.
template<class WebViewClientImpl, typename T>
class WebTestProxy : public WebViewClientImpl, public WebTestProxyBase {
public:
    explicit WebTestProxy(T t)
        : WebViewClientImpl(t)
    {
    }

    virtual ~WebTestProxy() { }

    virtual void didInvalidateRect(const WebKit::WebRect& rect)
    {
        WebTestProxyBase::didInvalidateRect(rect);
        WebViewClientImpl::didInvalidateRect(rect);
    }
    virtual void didScrollRect(int dx, int dy, const WebKit::WebRect& clipRect)
    {
        WebTestProxyBase::didScrollRect(dx, dy, clipRect);
        WebViewClientImpl::didScrollRect(dx, dy, clipRect);
    }
    virtual void scheduleComposite()
    {
        WebTestProxyBase::scheduleComposite();
        WebViewClientImpl::scheduleComposite();
    }
    virtual void scheduleAnimation()
    {
        WebTestProxyBase::scheduleAnimation();
        WebViewClientImpl::scheduleAnimation();
    }
    virtual void setWindowRect(const WebKit::WebRect& rect)
    {
        WebTestProxyBase::setWindowRect(rect);
        WebViewClientImpl::setWindowRect(rect);
    }
    virtual void show(WebKit::WebNavigationPolicy policy)
    {
        WebTestProxyBase::show(policy);
        WebViewClientImpl::show(policy);
    }
    virtual void didAutoResize(const WebKit::WebSize& newSize)
    {
        WebTestProxyBase::didAutoResize(newSize);
        WebViewClientImpl::didAutoResize(newSize);
    }
    virtual void postAccessibilityNotification(const WebKit::WebAccessibilityObject& object, WebKit::WebAccessibilityNotification notification)
    {
        WebTestProxyBase::postAccessibilityNotification(object, notification);
        WebViewClientImpl::postAccessibilityNotification(object, notification);
    }
    virtual void startDragging(WebKit::WebFrame* frame, const WebKit::WebDragData& data, WebKit::WebDragOperationsMask mask, const WebKit::WebImage& image, const WebKit::WebPoint& point)
    {
        WebTestProxyBase::startDragging(frame, data, mask, image, point);
        WebViewClientImpl::startDragging(frame, data, mask, image, point);
    }
    virtual bool shouldBeginEditing(const WebKit::WebRange& range)
    {
        WebTestProxyBase::shouldBeginEditing(range);
        return WebViewClientImpl::shouldBeginEditing(range);
    }
    virtual bool shouldEndEditing(const WebKit::WebRange& range)
    {
        WebTestProxyBase::shouldEndEditing(range);
        return WebViewClientImpl::shouldEndEditing(range);
    }
    virtual bool shouldInsertNode(const WebKit::WebNode& node, const WebKit::WebRange& range, WebKit::WebEditingAction action)
    {
        WebTestProxyBase::shouldInsertNode(node, range, action);
        return WebViewClientImpl::shouldInsertNode(node, range, action);
    }
    virtual bool shouldInsertText(const WebKit::WebString& text, const WebKit::WebRange& range, WebKit::WebEditingAction action)
    {
        WebTestProxyBase::shouldInsertText(text, range, action);
        return WebViewClientImpl::shouldInsertText(text, range, action);
    }
    virtual bool shouldChangeSelectedRange(const WebKit::WebRange& fromRange, const WebKit::WebRange& toRange, WebKit::WebTextAffinity affinity, bool stillSelecting)
    {
        WebTestProxyBase::shouldChangeSelectedRange(fromRange, toRange, affinity, stillSelecting);
        return WebViewClientImpl::shouldChangeSelectedRange(fromRange, toRange, affinity, stillSelecting);
    }
    virtual bool shouldDeleteRange(const WebKit::WebRange& range)
    {
        WebTestProxyBase::shouldDeleteRange(range);
        return WebViewClientImpl::shouldDeleteRange(range);
    }
    virtual bool shouldApplyStyle(const WebKit::WebString& style, const WebKit::WebRange& range)
    {
        WebTestProxyBase::shouldApplyStyle(style, range);
        return WebViewClientImpl::shouldApplyStyle(style, range);
    }
    virtual void didBeginEditing()
    {
        WebTestProxyBase::didBeginEditing();
        WebViewClientImpl::didBeginEditing();
    }
    virtual void didChangeSelection(bool isEmptySelection)
    {
        WebTestProxyBase::didChangeSelection(isEmptySelection);
        WebViewClientImpl::didChangeSelection(isEmptySelection);
    }
    virtual void didChangeContents()
    {
        WebTestProxyBase::didChangeContents();
        WebViewClientImpl::didChangeContents();
    }
    virtual void didEndEditing()
    {
        WebTestProxyBase::didEndEditing();
        WebViewClientImpl::didEndEditing();
    }
    virtual void registerIntentService(WebKit::WebFrame* frame, const WebKit::WebIntentServiceInfo& service)
    {
        WebTestProxyBase::registerIntentService(frame, service);
        WebViewClientImpl::registerIntentService(frame, service);
    }
    virtual void dispatchIntent(WebKit::WebFrame* source, const WebKit::WebIntentRequest& request)
    {
        WebTestProxyBase::dispatchIntent(source, request);
        WebViewClientImpl::dispatchIntent(source, request);
    }
};

}

#endif // WebTestProxy_h
