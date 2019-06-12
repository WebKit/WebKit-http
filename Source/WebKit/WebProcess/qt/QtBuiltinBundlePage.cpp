/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "QtBuiltinBundlePage.h"

#include "APIData.h"
#include "QtBuiltinBundle.h"
#include "WKArray.h"
#include "WKBundleFrame.h"
#include "WKData.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WKStringPrivate.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <QDebug>
#include <QJsonDocument>

namespace WebKit {

typedef JSClassRef (*CreateClassRefCallback)();

static void registerNavigatorObject(JSObjectRef *object, JSStringRef name,
                                    JSGlobalContextRef context, void* data,
                                    CreateClassRefCallback createClassRefCallback,
                                    JSStringRef postMessageName, JSObjectCallAsFunctionCallback postMessageCallback)
{
    static JSStringRef navigatorName = JSStringCreateWithUTF8CString("navigator");

    if (*object)
        JSValueUnprotect(context, *object);
    *object = JSObjectMake(context, createClassRefCallback(), data);
    JSValueProtect(context, *object);

    JSObjectRef postMessage = JSObjectMakeFunctionWithCallback(context, postMessageName, postMessageCallback);
    JSObjectSetProperty(context, *object, postMessageName, postMessage, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);

    JSValueRef navigatorValue = JSObjectGetProperty(context, JSContextGetGlobalObject(context), navigatorName, 0);
    if (!JSValueIsObject(context, navigatorValue))
        return;
    JSObjectRef navigatorObject = JSValueToObject(context, navigatorValue, 0);
    JSObjectSetProperty(context, navigatorObject, name, *object, kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);
}

static JSClassRef createEmptyJSClassRef()
{
    const JSClassDefinition definition = kJSClassDefinitionEmpty;
    return JSClassCreate(&definition);
}

static JSClassRef navigatorQtObjectClass()
{
    static JSClassRef classRef = createEmptyJSClassRef();
    return classRef;
}

static JSValueRef qt_postMessageCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
{
    // FIXME: should it work regardless of the thisObject?

    if (argumentCount < 1 || !JSValueIsString(context, arguments[0]))
        return JSValueMakeUndefined(context);

    QtBuiltinBundlePage* bundlePage = reinterpret_cast<QtBuiltinBundlePage*>(JSObjectGetPrivate(thisObject));
    ASSERT(bundlePage);

    // FIXME: needed?
    if (!bundlePage->navigatorQtObjectEnabled())
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> jsContents = JSValueToStringCopy(context, arguments[0], 0);
    WKRetainPtr<WKStringRef> contents(AdoptWK, WKStringCreateWithJSString(jsContents.get()));
    bundlePage->postMessageFromNavigatorQtObject(contents.get());
    return JSValueMakeUndefined(context);
}

static JSObjectRef createWrappedMessage(JSGlobalContextRef context, WKStringRef data)
{
    static JSStringRef dataName = JSStringCreateWithUTF8CString("data");

    JSRetainPtr<JSStringRef> jsData = WKStringCopyJSString(data);
    JSObjectRef wrappedMessage = JSObjectMake(context, 0, 0);
    JSObjectSetProperty(context, wrappedMessage, dataName, JSValueMakeString(context, jsData.get()), kJSPropertyAttributeDontDelete | kJSPropertyAttributeReadOnly, 0);
    return wrappedMessage;
}

static void callOnMessage(JSObjectRef object, WKStringRef contents, WKBundlePageRef page)
{
    static JSStringRef onmessageName = JSStringCreateWithUTF8CString("onmessage");

    if (!object)
        return;

    WKBundleFrameRef frame = WKBundlePageGetMainFrame(page);
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContext(frame);

    JSValueRef onmessageValue = JSObjectGetProperty(context, object, onmessageName, 0);
    if (!JSValueIsObject(context, onmessageValue))
        return;

    JSObjectRef onmessageFunction = JSValueToObject(context, onmessageValue, 0);
    if (!JSObjectIsFunction(context, onmessageFunction))
        return;

    JSObjectRef wrappedMessage = createWrappedMessage(context, contents);
    JSObjectCallAsFunction(context, onmessageFunction, 0, 1, &wrappedMessage, 0);
}

#if ENABLE(QT_WEBCHANNEL)
static JSClassRef navigatorQtWebChannelTransportObjectClass()
{
    static JSClassRef classRef = createEmptyJSClassRef();
    return classRef;
}

static QByteArray convertJsonTextToBinary(const CString& textJson)
{
    QByteArray jsonData = QByteArray::fromRawData(textJson.data(), textJson.length());

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse the client WebKit QWebChannel message as JSON: " << jsonData
            << "Error message is:" << error.errorString();
        return QByteArray();
    }
    if (!doc.isObject()) {
        qWarning() << "Received WebKit QWebChannel message is not a JSON object: " << jsonData;
        return QByteArray();
    }
    return doc.toBinaryData();
}

static JSValueRef qt_postWebChannelMessageCallback(JSContextRef context, JSObjectRef, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
{
    // FIXME: should it work regardless of the thisObject?

    if (argumentCount < 1 || !JSValueIsString(context, arguments[0]))
        return JSValueMakeUndefined(context);

    QtBuiltinBundlePage* bundlePage = reinterpret_cast<QtBuiltinBundlePage*>(JSObjectGetPrivate(thisObject));
    ASSERT(bundlePage);

    JSC::ExecState* exec = toJS(context);
    JSC::JSLockHolder locker(exec);

    JSC::JSValue jsValue = toJS(exec, arguments[0]);
    CString textJson = jsValue.toString(exec)->value(exec).utf8();

    QByteArray message = convertJsonTextToBinary(textJson);
    if (!message.isEmpty())
        bundlePage->postMessageFromNavigatorQtWebChannelTransport(WKDataCreate(reinterpret_cast<const unsigned char*>(message.data()), message.length()));

    return JSValueMakeUndefined(context);
}
#endif

QtBuiltinBundlePage::QtBuiltinBundlePage(QtBuiltinBundle* bundle, WKBundlePageRef page)
    : m_bundle(bundle)
    , m_page(page)
    , m_navigatorQtObject(0)
    , m_navigatorQtObjectEnabled(false)
#if ENABLE(QT_WEBCHANNEL)
    , m_navigatorQtWebChannelTransportObject(0)
#endif
{
    WKBundlePageLoaderClientV0 loaderClient;
    memset(&loaderClient, 0, sizeof(WKBundlePageLoaderClientV0));
    loaderClient.base.version = 0;
    loaderClient.base.clientInfo = this;
    loaderClient.didClearWindowObjectForFrame = didClearWindowForFrame;
    WKBundlePageSetPageLoaderClient(m_page, &loaderClient.base);
}

QtBuiltinBundlePage::~QtBuiltinBundlePage()
{
    if (!m_navigatorQtObject
#if ENABLE(QT_WEBCHANNEL)
        && !m_navigatorQtWebChannelTransportObject
#endif
    )
    {
        return;
    }

    WKBundleFrameRef frame = WKBundlePageGetMainFrame(m_page);
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContext(frame);

    if (m_navigatorQtObject)
        JSValueUnprotect(context, m_navigatorQtObject);

#if ENABLE(QT_WEBCHANNEL)
    if (m_navigatorQtWebChannelTransportObject)
        JSValueUnprotect(context, m_navigatorQtWebChannelTransportObject);
#endif
}

void QtBuiltinBundlePage::didClearWindowForFrame(WKBundlePageRef page, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void* clientInfo)
{
    static_cast<QtBuiltinBundlePage*>(const_cast<void*>(clientInfo))->didClearWindowForFrame(frame, world);
}

void QtBuiltinBundlePage::didClearWindowForFrame(WKBundleFrameRef frame, WKBundleScriptWorldRef world)
{
    if (!WKBundleFrameIsMainFrame(frame) || WKBundleScriptWorldNormalWorld() != world)
        return;
    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, world);
    registerNavigatorQtObject(context);
#if ENABLE(QT_WEBCHANNEL)
    registerNavigatorQtWebChannelTransportObject(context);
#endif
}

void QtBuiltinBundlePage::postMessageFromNavigatorQtObject(WKStringRef message)
{
    static WKStringRef messageName = WKStringCreateWithUTF8CString("MessageFromNavigatorQtObject");
    postNavigatorMessage(messageName, message);
}

void QtBuiltinBundlePage::didReceiveMessageToNavigatorQtObject(WKStringRef message)
{
    callOnMessage(m_navigatorQtObject, message, m_page);
}

void QtBuiltinBundlePage::setNavigatorQtObjectEnabled(bool enabled)
{
    if (enabled == m_navigatorQtObjectEnabled)
        return;
    // Note that this will take effect only after the next page load.
    m_navigatorQtObjectEnabled = enabled;
}

void QtBuiltinBundlePage::registerNavigatorQtObject(JSGlobalContextRef context)
{
    static JSStringRef name = JSStringCreateWithUTF8CString("qt");
    static JSStringRef postMessageName = JSStringCreateWithUTF8CString("postMessage");
    registerNavigatorObject(&m_navigatorQtObject, name, context, this,
                            &navigatorQtObjectClass,
                            postMessageName, &qt_postMessageCallback);
}

#if ENABLE(QT_WEBCHANNEL)
void QtBuiltinBundlePage::registerNavigatorQtWebChannelTransportObject(JSGlobalContextRef context)
{
    static JSStringRef name = JSStringCreateWithUTF8CString("qtWebChannelTransport");
    static JSStringRef postMessageName = JSStringCreateWithUTF8CString("send");
    registerNavigatorObject(&m_navigatorQtWebChannelTransportObject, name, context, this,
                            &navigatorQtWebChannelTransportObjectClass,
                            postMessageName, &qt_postWebChannelMessageCallback);
}

void QtBuiltinBundlePage::didReceiveMessageToNavigatorQtWebChannelTransport(WKDataRef contents)
{
    const char* bytes = reinterpret_cast<const char*>(WKDataGetBytes(contents));
    int size = WKDataGetSize(contents);
    QJsonDocument doc = QJsonDocument::fromRawData(bytes, size, QJsonDocument::BypassValidation);
    ASSERT(doc.isObject());

    QByteArray textJson = doc.toJson(QJsonDocument::Compact);
    WKStringRef message = WKStringCreateWithUTF8CString(textJson.constData());
    callOnMessage(m_navigatorQtWebChannelTransportObject, message, m_page);
}

void QtBuiltinBundlePage::postMessageFromNavigatorQtWebChannelTransport(WKDataRef message)
{
    static WKStringRef messageName = WKStringCreateWithUTF8CString("MessageFromNavigatorQtWebChannelTransportObject");
    postNavigatorMessage(messageName, message);
}
#endif

void QtBuiltinBundlePage::postNavigatorMessage(WKStringRef messageName, WKTypeRef message)
{
    WKTypeRef body[] = { page(), message };
    WKRetainPtr<WKArrayRef> messageBody(AdoptWK, WKArrayCreate(body, sizeof(body) / sizeof(WKTypeRef)));
    WKBundlePostMessage(m_bundle->toRef(), messageName, messageBody.get());
}

} // namespace WebKit
