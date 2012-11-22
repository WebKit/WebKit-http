/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "PageUIClientEfl.h"

#include "EwkViewImpl.h"
#include "WKAPICast.h"
#include "WKEvent.h"
#include "WKString.h"
#include "ewk_file_chooser_request_private.h"
#include "ewk_window_features_private.h"
#include <Ecore_Evas.h>
#include <WebCore/Color.h>

using namespace EwkViewCallbacks;

namespace WebKit {

static inline PageUIClientEfl* toPageUIClientEfl(const void* clientInfo)
{
    return static_cast<PageUIClientEfl*>(const_cast<void*>(clientInfo));
}

void PageUIClientEfl::closePage(WKPageRef, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_viewImpl->closePage();
}

WKPageRef PageUIClientEfl::createNewPage(WKPageRef, WKURLRequestRef, WKDictionaryRef wkWindowFeatures, WKEventModifiers, WKEventMouseButton, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_viewImpl->createNewPage(toImpl(wkWindowFeatures));
}

void PageUIClientEfl::runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_viewImpl->requestJSAlertPopup(WKEinaSharedString(alertText));
}

bool PageUIClientEfl::runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_viewImpl->requestJSConfirmPopup(WKEinaSharedString(message));
}

WKStringRef PageUIClientEfl::runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    WKEinaSharedString value = toPageUIClientEfl(clientInfo)->m_viewImpl->requestJSPromptPopup(WKEinaSharedString(message), WKEinaSharedString(defaultValue));
    return value ? WKStringCreateWithUTF8CString(value) : 0;
}

bool PageUIClientEfl::toolbarsAreVisible(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    return features->toolbarVisible();
}

void PageUIClientEfl::setToolbarsAreVisible(WKPageRef, bool toolbarVisible, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    features->setToolbarVisible(toolbarVisible);
}

bool PageUIClientEfl::menuBarIsVisible(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    return features->menuBarVisible();
}

void PageUIClientEfl::setMenuBarIsVisible(WKPageRef, bool menuBarVisible, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    features->setMenuBarVisible(menuBarVisible);
}

bool PageUIClientEfl::statusBarIsVisible(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    return features->statusBarVisible();
}

void PageUIClientEfl::setStatusBarIsVisible(WKPageRef, bool statusBarVisible, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    features->setStatusBarVisible(statusBarVisible);
}

bool PageUIClientEfl::isResizable(WKPageRef, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    return features->resizable();
}

void PageUIClientEfl::setIsResizable(WKPageRef, bool resizable, const void* clientInfo)
{
    EwkWindowFeatures* features = toPageUIClientEfl(clientInfo)->m_viewImpl->windowFeatures();
    ASSERT(features);
    features->setResizable(resizable);
}

#if ENABLE(INPUT_TYPE_COLOR)
void PageUIClientEfl::showColorPicker(WKPageRef, WKStringRef initialColor, WKColorPickerResultListenerRef listener, const void* clientInfo)
{
    PageUIClientEfl* pageUIClient = toPageUIClientEfl(clientInfo);
    WebCore::Color color = WebCore::Color(WebKit::toWTFString(initialColor));
    pageUIClient->m_viewImpl->requestColorPicker(listener, color);
}

void PageUIClientEfl::hideColorPicker(WKPageRef, const void* clientInfo)
{
    PageUIClientEfl* pageUIClient = toPageUIClientEfl(clientInfo);
    pageUIClient->m_viewImpl->dismissColorPicker();
}
#endif

#if ENABLE(SQL_DATABASE)
unsigned long long PageUIClientEfl::exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef databaseName, WKStringRef displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageUIClientEfl(clientInfo)->m_viewImpl;
    return viewImpl->informDatabaseQuotaReached(toImpl(databaseName)->string(), toImpl(displayName)->string(), currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);
}
#endif

void PageUIClientEfl::focus(WKPageRef, const void* clientInfo)
{
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), true);
}

void PageUIClientEfl::unfocus(WKPageRef, const void* clientInfo)
{
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), false);
}

void PageUIClientEfl::takeFocus(WKPageRef, WKFocusDirection, const void* clientInfo)
{
    // FIXME: this is only a partial implementation.
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), false);
}

WKRect PageUIClientEfl::getWindowFrame(WKPageRef, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_viewImpl->windowGeometry();
}

void PageUIClientEfl::setWindowFrame(WKPageRef, WKRect frame, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_viewImpl->setWindowGeometry(frame);
}

void PageUIClientEfl::runOpenPanel(WKPageRef, WKFrameRef, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageUIClientEfl(clientInfo)->m_viewImpl;
    RefPtr<EwkFileChooserRequest> fileChooserRequest = EwkFileChooserRequest::create(toImpl(parameters), toImpl(listener));
    viewImpl->smartCallback<FileChooserRequest>().call(fileChooserRequest.get());
}

PageUIClientEfl::PageUIClientEfl(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
{
    WKPageRef pageRef = m_viewImpl->wkPage();
    ASSERT(pageRef);

    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(WKPageUIClient));
    uiClient.version = kWKPageUIClientCurrentVersion;
    uiClient.clientInfo = this;
    uiClient.close = closePage;
    uiClient.createNewPage = createNewPage;
    uiClient.runJavaScriptAlert = runJavaScriptAlert;
    uiClient.runJavaScriptConfirm = runJavaScriptConfirm;
    uiClient.runJavaScriptPrompt = runJavaScriptPrompt;
    uiClient.toolbarsAreVisible = toolbarsAreVisible;
    uiClient.setToolbarsAreVisible = setToolbarsAreVisible;
    uiClient.menuBarIsVisible = menuBarIsVisible;
    uiClient.setMenuBarIsVisible = setMenuBarIsVisible;
    uiClient.statusBarIsVisible = statusBarIsVisible;
    uiClient.setStatusBarIsVisible = setStatusBarIsVisible;
    uiClient.isResizable = isResizable;
    uiClient.setIsResizable = setIsResizable;
    uiClient.takeFocus = takeFocus;
    uiClient.focus = focus;
    uiClient.unfocus = unfocus;
    uiClient.getWindowFrame = getWindowFrame;
    uiClient.setWindowFrame = setWindowFrame;
    uiClient.runOpenPanel = runOpenPanel;
#if ENABLE(SQL_DATABASE)
    uiClient.exceededDatabaseQuota = exceededDatabaseQuota;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    uiClient.showColorPicker = showColorPicker;
    uiClient.hideColorPicker = hideColorPicker;
#endif

    WKPageSetPageUIClient(pageRef, &uiClient);
}

} // namespace WebKit
