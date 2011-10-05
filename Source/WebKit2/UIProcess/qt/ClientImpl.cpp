/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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
*/

#include "config.h"
#include "ClientImpl.h"

#include "WebFrameProxy.h"
#include "WKAPICast.h"
#include "WKStringQt.h"
#include "WKURLQt.h"
#include "qweberror.h"
#include "qweberror_p.h"
#include <PolicyInterface.h>
#include <QtWebPageProxy.h>
#include <ViewInterface.h>
#include <WKArray.h>
#include <WKFrame.h>
#include <WKFramePolicyListener.h>
#include <WKHitTestResult.h>
#include <WKOpenPanelParameters.h>
#include <WKOpenPanelResultListener.h>
#include <WKType.h>
#include <WKURLRequest.h>

using namespace WebKit;

static QtWebPageProxy* toQtWebPageProxy(const void* clientInfo)
{
    if (clientInfo)
        return reinterpret_cast<QtWebPageProxy*>(const_cast<void*>(clientInfo));
    return 0;
}

static inline ViewInterface* toViewInterface(const void* clientInfo)
{
    ASSERT(clientInfo);
    return reinterpret_cast<ViewInterface*>(const_cast<void*>(clientInfo));
}

static inline PolicyInterface* toPolicyInterface(const void* clientInfo)
{
    ASSERT(clientInfo);
    return reinterpret_cast<PolicyInterface*>(const_cast<void*>(clientInfo));
}

static void dispatchLoadSucceeded(WKFrameRef frame, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->loadDidSucceed();
}

static void dispatchLoadFailed(WKFrameRef frame, const void* clientInfo, WKErrorRef error)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->loadDidFail(QWebErrorPrivate::createQWebError(error));
}

void qt_wk_didStartProvisionalLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->loadDidBegin();
}

void qt_wk_didFailProvisionalLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef userData, const void* clientInfo)
{
    dispatchLoadFailed(frame, clientInfo, error);
}

void qt_wk_didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;
    WebFrameProxy* wkframe = toImpl(frame);
    QString urlStr(wkframe->url());
    QUrl qUrl = urlStr;
    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->didChangeUrl(qUrl);
    toQtWebPageProxy(clientInfo)->loadDidCommit();
}

void qt_wk_didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    dispatchLoadSucceeded(frame, clientInfo);
}

void qt_wk_didFailLoadWithErrorForFrame(WKPageRef page, WKFrameRef frame, WKErrorRef error, WKTypeRef userData, const void* clientInfo)
{
    dispatchLoadFailed(frame, clientInfo, error);
}

void qt_wk_didSameDocumentNavigationForFrame(WKPageRef page, WKFrameRef frame, WKSameDocumentNavigationType type, WKTypeRef userData, const void* clientInfo)
{
    WebFrameProxy* wkframe = toImpl(frame);
    QString urlStr(wkframe->url());
    QUrl qUrl = urlStr;
    toQtWebPageProxy(clientInfo)->updateNavigationActions();
    toQtWebPageProxy(clientInfo)->didChangeUrl(qUrl);
}

void qt_wk_didReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;
    QString qTitle = WKStringCopyQString(title);
    toQtWebPageProxy(clientInfo)->didChangeTitle(qTitle);
}

void qt_wk_didStartProgress(WKPageRef page, const void* clientInfo)
{
    toQtWebPageProxy(clientInfo)->didChangeLoadProgress(0);
}

void qt_wk_didChangeProgress(WKPageRef page, const void* clientInfo)
{
    toQtWebPageProxy(clientInfo)->didChangeLoadProgress(WKPageGetEstimatedProgress(page) * 100);
}

void qt_wk_didFinishProgress(WKPageRef page, const void* clientInfo)
{
    toQtWebPageProxy(clientInfo)->didChangeLoadProgress(100);
}

void qt_wk_runJavaScriptAlert(WKPageRef page, WKStringRef alertText, WKFrameRef frame, const void* clientInfo)
{
    QString qAlertText = WKStringCopyQString(alertText);
    toViewInterface(clientInfo)->runJavaScriptAlert(qAlertText);
}

bool qt_wk_runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    QString qMessage = WKStringCopyQString(message);
    return toViewInterface(clientInfo)->runJavaScriptConfirm(qMessage);
}

static inline WKStringRef createNullWKString()
{
    RefPtr<WebString> webString = WebString::createNull();
    return toAPI(webString.release().leakRef());
}

WKStringRef qt_wk_runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    QString qMessage = WKStringCopyQString(message);
    QString qDefaultValue = WKStringCopyQString(defaultValue);
    bool ok = false;
    QString result = toViewInterface(clientInfo)->runJavaScriptPrompt(qMessage, qDefaultValue, ok);
    if (!ok)
        return createNullWKString();
    return WKStringCreateWithQString(result);
}

void qt_wk_setStatusText(WKPageRef, WKStringRef text, const void *clientInfo)
{
    QString qText = WKStringCopyQString(text);
    toViewInterface(clientInfo)->didChangeStatusText(qText);
}

void qt_wk_runOpenPanel(WKPageRef, WKFrameRef, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    Vector<String> wkSelectedFileNames = toImpl(parameters)->selectedFileNames();

    QStringList selectedFileNames;
    if (!wkSelectedFileNames.isEmpty())
        for (unsigned i = 0; wkSelectedFileNames.size(); ++i)
            selectedFileNames += wkSelectedFileNames.at(i);

    ViewInterface::FileChooserType allowMultipleFiles = WKOpenPanelParametersGetAllowsMultipleFiles(parameters) ? ViewInterface::MultipleFilesSelection : ViewInterface::SingleFileSelection;
    toViewInterface(clientInfo)->chooseFiles(listener, selectedFileNames, allowMultipleFiles);
}

void qt_wk_mouseDidMoveOverElement(WKPageRef page, WKHitTestResultRef hitTestResult, WKEventModifiers modifiers, WKTypeRef userData, const void* clientInfo)
{
    const QUrl absoluteLinkUrl = WKURLCopyQUrl(WKHitTestResultCopyAbsoluteLinkURL(hitTestResult));
    const QString linkTitle = WKStringCopyQString(WKHitTestResultCopyLinkTitle(hitTestResult));
    toViewInterface(clientInfo)->didMouseMoveOverElement(absoluteLinkUrl, linkTitle);
}

static Qt::MouseButton toQtMouseButton(WKEventMouseButton button)
{
    switch (button) {
    case kWKEventMouseButtonLeftButton:
        return Qt::LeftButton;
    case kWKEventMouseButtonMiddleButton:
        return Qt::MiddleButton;
    case kWKEventMouseButtonRightButton:
        return Qt::RightButton;
    }
    return Qt::NoButton;
}

static Qt::KeyboardModifiers toQtKeyboardModifiers(WKEventModifiers modifiers)
{
    Qt::KeyboardModifiers qtModifiers = Qt::NoModifier;
    if (modifiers & kWKEventModifiersShiftKey)
        qtModifiers |= Qt::ShiftModifier;
    if (modifiers & kWKEventModifiersControlKey)
        qtModifiers |= Qt::ControlModifier;
    if (modifiers & kWKEventModifiersAltKey)
        qtModifiers |= Qt::AltModifier;
    if (modifiers & kWKEventModifiersMetaKey)
        qtModifiers |= Qt::MetaModifier;
    return qtModifiers;
}

void qt_wk_decidePolicyForNavigationAction(WKPageRef page, WKFrameRef frame, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    PolicyInterface* policyInterface = toPolicyInterface(clientInfo);
    WKURLRef requestURL = WKURLRequestCopyURL(request);
    QUrl qUrl = WKURLCopyQUrl(requestURL);
    WKRelease(requestURL);

    PolicyInterface::PolicyAction action = policyInterface->navigationPolicyForURL(qUrl, toQtMouseButton(mouseButton), toQtKeyboardModifiers(modifiers));
    switch (action) {
    case PolicyInterface::Use:
        WKFramePolicyListenerUse(listener);
        break;
    case PolicyInterface::Download:
        WKFramePolicyListenerDownload(listener);
        break;
    case PolicyInterface::Ignore:
        WKFramePolicyListenerIgnore(listener);
        break;
    }
}
