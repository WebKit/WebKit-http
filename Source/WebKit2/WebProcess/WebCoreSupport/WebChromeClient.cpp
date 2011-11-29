/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "WebChromeClient.h"

#include "DrawingArea.h"
#include "InjectedBundleNavigationAction.h"
#include "InjectedBundleUserMessageCoders.h"
#include "WebContextMenu.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebFrameLoaderClient.h"
#include "WebFullScreenManager.h"
#include "WebOpenPanelParameters.h"
#include "WebOpenPanelResultListener.h"
#include "WebPage.h"
#include "WebPageCreationParameters.h"
#include "WebPageProxyMessages.h"
#include "WebPopupMenu.h"
#include "WebPreferencesStore.h"
#include "WebProcess.h"
#include "WebSearchPopupMenu.h"
#include <WebCore/AXObjectCache.h>
#include <WebCore/DatabaseTracker.h>
#include <WebCore/FileChooser.h>
#include <WebCore/FileIconLoader.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLPlugInImageElement.h>
#include <WebCore/Icon.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/Settings.h>

using namespace WebCore;
using namespace HTMLNames;

namespace WebKit {

static double area(WebFrame* frame)
{
    IntSize size = frame->visibleContentBoundsExcludingScrollbars().size();
    return static_cast<double>(size.height()) * size.width();
}


static WebFrame* findLargestFrameInFrameSet(WebPage* page)
{
    // Approximate what a user could consider a default target frame for application menu operations.

    WebFrame* mainFrame = page->mainWebFrame();
    if (!mainFrame->isFrameSet())
        return 0;

    WebFrame* largestSoFar = 0;

    RefPtr<ImmutableArray> frameChildren = mainFrame->childFrames();
    size_t count = frameChildren->size();
    for (size_t i = 0; i < count; ++i) {
        WebFrame* childFrame = frameChildren->at<WebFrame>(i);
        if (!largestSoFar || area(childFrame) > area(largestSoFar))
            largestSoFar = childFrame;
    }

    return largestSoFar;
}

void WebChromeClient::chromeDestroyed()
{
    delete this;
}

void WebChromeClient::setWindowRect(const FloatRect& windowFrame)
{
    m_page->send(Messages::WebPageProxy::SetWindowFrame(windowFrame));
}

FloatRect WebChromeClient::windowRect()
{
    FloatRect newWindowFrame;

    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::GetWindowFrame(), Messages::WebPageProxy::GetWindowFrame::Reply(newWindowFrame), m_page->pageID()))
        return FloatRect();

    return newWindowFrame;
}

FloatRect WebChromeClient::pageRect()
{
    return FloatRect(FloatPoint(), m_page->size());
}

void WebChromeClient::focus()
{
    m_page->send(Messages::WebPageProxy::SetFocus(true));
}

void WebChromeClient::unfocus()
{
    m_page->send(Messages::WebPageProxy::SetFocus(false));
}

#if PLATFORM(MAC)
void WebChromeClient::makeFirstResponder()
{
    m_page->send(Messages::WebPageProxy::MakeFirstResponder());
}    
#endif    

bool WebChromeClient::canTakeFocus(FocusDirection)
{
    notImplemented();
    return true;
}

void WebChromeClient::takeFocus(FocusDirection direction)
{
    m_page->send(Messages::WebPageProxy::TakeFocus(direction));
}

void WebChromeClient::focusedNodeChanged(Node*)
{
    notImplemented();
}

void WebChromeClient::focusedFrameChanged(Frame* frame)
{
    WebFrame* webFrame = frame ? static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame() : 0;

    WebProcess::shared().connection()->send(Messages::WebPageProxy::FocusedFrameChanged(webFrame ? webFrame->frameID() : 0), m_page->pageID());
}

Page* WebChromeClient::createWindow(Frame*, const FrameLoadRequest& request, const WindowFeatures& windowFeatures, const NavigationAction& navigationAction)
{
    uint32_t modifiers = static_cast<uint32_t>(InjectedBundleNavigationAction::modifiersForNavigationAction(navigationAction));
    int32_t mouseButton = static_cast<int32_t>(InjectedBundleNavigationAction::mouseButtonForNavigationAction(navigationAction));

    uint64_t newPageID = 0;
    WebPageCreationParameters parameters;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::CreateNewPage(request.resourceRequest(), windowFeatures, modifiers, mouseButton), Messages::WebPageProxy::CreateNewPage::Reply(newPageID, parameters), m_page->pageID()))
        return 0;

    if (!newPageID)
        return 0;

    WebProcess::shared().createWebPage(newPageID, parameters);
    return WebProcess::shared().webPage(newPageID)->corePage();
}

void WebChromeClient::show()
{
    m_page->show();
}

bool WebChromeClient::canRunModal()
{
    return m_page->canRunModal();
}

void WebChromeClient::runModal()
{
    m_page->runModal();
}

void WebChromeClient::setToolbarsVisible(bool toolbarsAreVisible)
{
    m_page->send(Messages::WebPageProxy::SetToolbarsAreVisible(toolbarsAreVisible));
}

bool WebChromeClient::toolbarsVisible()
{
    WKBundlePageUIElementVisibility toolbarsVisibility = m_page->injectedBundleUIClient().toolbarsAreVisible(m_page);
    if (toolbarsVisibility != WKBundlePageUIElementVisibilityUnknown)
        return toolbarsVisibility == WKBundlePageUIElementVisible;
    
    bool toolbarsAreVisible = true;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::GetToolbarsAreVisible(), Messages::WebPageProxy::GetToolbarsAreVisible::Reply(toolbarsAreVisible), m_page->pageID()))
        return true;

    return toolbarsAreVisible;
}

void WebChromeClient::setStatusbarVisible(bool statusBarIsVisible)
{
    m_page->send(Messages::WebPageProxy::SetStatusBarIsVisible(statusBarIsVisible));
}

bool WebChromeClient::statusbarVisible()
{
    WKBundlePageUIElementVisibility statusbarVisibility = m_page->injectedBundleUIClient().statusBarIsVisible(m_page);
    if (statusbarVisibility != WKBundlePageUIElementVisibilityUnknown)
        return statusbarVisibility == WKBundlePageUIElementVisible;

    bool statusBarIsVisible = true;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::GetStatusBarIsVisible(), Messages::WebPageProxy::GetStatusBarIsVisible::Reply(statusBarIsVisible), m_page->pageID()))
        return true;

    return statusBarIsVisible;
}

void WebChromeClient::setScrollbarsVisible(bool)
{
    notImplemented();
}

bool WebChromeClient::scrollbarsVisible()
{
    notImplemented();
    return true;
}

void WebChromeClient::setMenubarVisible(bool menuBarVisible)
{
    m_page->send(Messages::WebPageProxy::SetMenuBarIsVisible(menuBarVisible));
}

bool WebChromeClient::menubarVisible()
{
    WKBundlePageUIElementVisibility menubarVisibility = m_page->injectedBundleUIClient().menuBarIsVisible(m_page);
    if (menubarVisibility != WKBundlePageUIElementVisibilityUnknown)
        return menubarVisibility == WKBundlePageUIElementVisible;
    
    bool menuBarIsVisible = true;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::GetMenuBarIsVisible(), Messages::WebPageProxy::GetMenuBarIsVisible::Reply(menuBarIsVisible), m_page->pageID()))
        return true;

    return menuBarIsVisible;
}

void WebChromeClient::setResizable(bool resizable)
{
    m_page->send(Messages::WebPageProxy::SetIsResizable(resizable));
}

void WebChromeClient::addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message, unsigned int lineNumber, const String& sourceID)
{
    // Notify the bundle client.
    m_page->injectedBundleUIClient().willAddMessageToConsole(m_page, message, lineNumber);

    notImplemented();
}

bool WebChromeClient::canRunBeforeUnloadConfirmPanel()
{
    return m_page->canRunBeforeUnloadConfirmPanel();
}

bool WebChromeClient::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();

    bool shouldClose = false;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::RunBeforeUnloadConfirmPanel(message, webFrame->frameID()), Messages::WebPageProxy::RunBeforeUnloadConfirmPanel::Reply(shouldClose), m_page->pageID()))
        return false;

    return shouldClose;
}

void WebChromeClient::closeWindowSoon()
{
    // FIXME: This code assumes that the client will respond to a close page
    // message by actually closing the page. Safari does this, but there is
    // no guarantee that other applications will, which will leave this page
    // half detached. This approach is an inherent limitation making parts of
    // a close execute synchronously as part of window.close, but other parts
    // later on.

    m_page->corePage()->setGroupName(String());

    if (WebFrame* frame = m_page->mainWebFrame()) {
        if (Frame* coreFrame = frame->coreFrame())
            coreFrame->loader()->stopForUserCancel();
    }

    m_page->sendClose();
}

void WebChromeClient::runJavaScriptAlert(Frame* frame, const String& alertText)
{
    WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();

    // Notify the bundle client.
    m_page->injectedBundleUIClient().willRunJavaScriptAlert(m_page, alertText, webFrame);

    unsigned syncSendFlags = (WebCore::AXObjectCache::accessibilityEnabled()) ? CoreIPC::SpinRunLoopWhileWaitingForReply : 0;
    WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::RunJavaScriptAlert(webFrame->frameID(), alertText), Messages::WebPageProxy::RunJavaScriptAlert::Reply(), m_page->pageID(), CoreIPC::Connection::DefaultTimeout, syncSendFlags);
}

bool WebChromeClient::runJavaScriptConfirm(Frame* frame, const String& message)
{
    WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();

    // Notify the bundle client.
    m_page->injectedBundleUIClient().willRunJavaScriptConfirm(m_page, message, webFrame);

    unsigned syncSendFlags = (WebCore::AXObjectCache::accessibilityEnabled()) ? CoreIPC::SpinRunLoopWhileWaitingForReply : 0;
    bool result = false;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::RunJavaScriptConfirm(webFrame->frameID(), message), Messages::WebPageProxy::RunJavaScriptConfirm::Reply(result), m_page->pageID(), CoreIPC::Connection::DefaultTimeout, syncSendFlags))
        return false;

    return result;
}

bool WebChromeClient::runJavaScriptPrompt(Frame* frame, const String& message, const String& defaultValue, String& result)
{
    WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();

    // Notify the bundle client.
    m_page->injectedBundleUIClient().willRunJavaScriptPrompt(m_page, message, defaultValue, webFrame);

    unsigned syncSendFlags = (WebCore::AXObjectCache::accessibilityEnabled()) ? CoreIPC::SpinRunLoopWhileWaitingForReply : 0;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::RunJavaScriptPrompt(webFrame->frameID(), message, defaultValue), Messages::WebPageProxy::RunJavaScriptPrompt::Reply(result), m_page->pageID(), CoreIPC::Connection::DefaultTimeout, syncSendFlags))
        return false;

    return !result.isNull();
}

void WebChromeClient::setStatusbarText(const String& statusbarText)
{
    // Notify the bundle client.
    m_page->injectedBundleUIClient().willSetStatusbarText(m_page, statusbarText);

    m_page->send(Messages::WebPageProxy::SetStatusText(statusbarText));
}

bool WebChromeClient::shouldInterruptJavaScript()
{
    bool shouldInterrupt = false;
    if (!WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::ShouldInterruptJavaScript(), Messages::WebPageProxy::ShouldInterruptJavaScript::Reply(shouldInterrupt), m_page->pageID()))
        return false;

    return shouldInterrupt;
}

KeyboardUIMode WebChromeClient::keyboardUIMode()
{
    return m_page->keyboardUIMode();
}

IntRect WebChromeClient::windowResizerRect() const
{
    return m_page->windowResizerRect();
}

void WebChromeClient::invalidateRootView(const IntRect&, bool)
{
    // Do nothing here, there's no concept of invalidating the window in the web process.
}

void WebChromeClient::invalidateContentsAndRootView(const IntRect& rect, bool)
{
    if (Document* document = m_page->corePage()->mainFrame()->document()) {
        if (document->printing())
            return;
    }

    m_page->drawingArea()->setNeedsDisplay(rect);
}

void WebChromeClient::invalidateContentsForSlowScroll(const IntRect& rect, bool)
{
    if (Document* document = m_page->corePage()->mainFrame()->document()) {
        if (document->printing())
            return;
    }

    m_page->pageDidScroll();
    m_page->drawingArea()->setNeedsDisplay(rect);
}

void WebChromeClient::scroll(const IntSize& scrollOffset, const IntRect& scrollRect, const IntRect& clipRect)
{
    m_page->pageDidScroll();
    m_page->drawingArea()->scroll(intersection(scrollRect, clipRect), scrollOffset);
}

#if USE(TILED_BACKING_STORE)
void WebChromeClient::delegatedScrollRequested(const IntPoint& scrollOffset)
{
    m_page->pageDidRequestScroll(scrollOffset);
}
#endif

IntPoint WebChromeClient::screenToRootView(const IntPoint& point) const
{
    return m_page->screenToWindow(point);
}

IntRect WebChromeClient::rootViewToScreen(const IntRect& rect) const
{
    return m_page->windowToScreen(rect);
}

PlatformPageClient WebChromeClient::platformPageClient() const
{
    notImplemented();
    return 0;
}

void WebChromeClient::contentsSizeChanged(Frame* frame, const IntSize& size) const
{
    if (!m_page->corePage()->settings()->frameFlatteningEnabled()) {
        WebFrame* largestFrame = findLargestFrameInFrameSet(m_page);
        if (largestFrame != m_cachedFrameSetLargestFrame.get()) {
            m_cachedFrameSetLargestFrame = largestFrame;
            m_page->send(Messages::WebPageProxy::FrameSetLargestFrameChanged(largestFrame ? largestFrame->frameID() : 0));
        }
    }

    if (frame->page()->mainFrame() != frame)
        return;

#if PLATFORM(QT)
    m_page->send(Messages::WebPageProxy::DidChangeContentsSize(size));

    if (m_page->useFixedLayout())
        m_page->resizeToContentsIfNeeded();
#endif

    FrameView* frameView = frame->view();
    if (frameView && !frameView->delegatesScrolling())  {
        bool hasHorizontalScrollbar = frameView->horizontalScrollbar();
        bool hasVerticalScrollbar = frameView->verticalScrollbar();

        if (hasHorizontalScrollbar != m_cachedMainFrameHasHorizontalScrollbar || hasVerticalScrollbar != m_cachedMainFrameHasVerticalScrollbar) {
            m_page->send(Messages::WebPageProxy::DidChangeScrollbarsForMainFrame(hasHorizontalScrollbar, hasVerticalScrollbar));

            m_cachedMainFrameHasHorizontalScrollbar = hasHorizontalScrollbar;
            m_cachedMainFrameHasVerticalScrollbar = hasVerticalScrollbar;
        }
    }
}

void WebChromeClient::scrollRectIntoView(const IntRect&) const
{
    notImplemented();
}

bool WebChromeClient::shouldMissingPluginMessageBeButton() const
{
    // FIXME: <rdar://problem/8794397> We should only return true when there is a 
    // missingPluginButtonClicked callback defined on the Page UI client.
    return true;
}
    
void WebChromeClient::missingPluginButtonClicked(Element* element) const
{
    ASSERT(element->hasTagName(objectTag) || element->hasTagName(embedTag));

    HTMLPlugInImageElement* pluginElement = static_cast<HTMLPlugInImageElement*>(element);

    m_page->send(Messages::WebPageProxy::MissingPluginButtonClicked(pluginElement->serviceType(), pluginElement->url(), pluginElement->getAttribute(pluginspageAttr)));
}

void WebChromeClient::scrollbarsModeDidChange() const
{
    notImplemented();
}

void WebChromeClient::mouseDidMoveOverElement(const HitTestResult& hitTestResult, unsigned modifierFlags)
{
    RefPtr<APIObject> userData;

    // Notify the bundle client.
    m_page->injectedBundleUIClient().mouseDidMoveOverElement(m_page, hitTestResult, static_cast<WebEvent::Modifiers>(modifierFlags), userData);

    WebHitTestResult::Data webHitTestResultData;
    webHitTestResultData.absoluteImageURL = hitTestResult.absoluteImageURL().string();
    webHitTestResultData.absolutePDFURL = hitTestResult.absolutePDFURL().string();
    webHitTestResultData.absoluteLinkURL = hitTestResult.absoluteLinkURL().string();
    webHitTestResultData.absoluteMediaURL = hitTestResult.absoluteMediaURL().string();
    webHitTestResultData.linkLabel = hitTestResult.textContent();
    webHitTestResultData.linkTitle = hitTestResult.titleDisplayString();

    // Notify the UIProcess.
    m_page->send(Messages::WebPageProxy::MouseDidMoveOverElement(webHitTestResultData, modifierFlags, InjectedBundleUserMessageEncoder(userData.get())));
}

void WebChromeClient::setToolTip(const String& toolTip, TextDirection)
{
    // Only send a tool tip to the WebProcess if it has changed since the last time this function was called.

    if (toolTip == m_cachedToolTip)
        return;
    m_cachedToolTip = toolTip;

    m_page->send(Messages::WebPageProxy::SetToolTip(m_cachedToolTip));
}

void WebChromeClient::print(Frame* frame)
{
    WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();
    m_page->sendSync(Messages::WebPageProxy::PrintFrame(webFrame->frameID()), Messages::WebPageProxy::PrintFrame::Reply());
}

#if ENABLE(SQL_DATABASE)
void WebChromeClient::exceededDatabaseQuota(Frame* frame, const String& databaseName)
{
    WebFrame* webFrame = static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame();
    SecurityOrigin* origin = frame->document()->securityOrigin();

    DatabaseDetails details = DatabaseTracker::tracker().detailsForNameAndOrigin(databaseName, origin);
    uint64_t currentQuota = DatabaseTracker::tracker().quotaForOrigin(origin);
    uint64_t currentOriginUsage = DatabaseTracker::tracker().usageForOrigin(origin);
    uint64_t newQuota = 0;
    WebProcess::shared().connection()->sendSync(
        Messages::WebPageProxy::ExceededDatabaseQuota(webFrame->frameID(), origin->databaseIdentifier(), databaseName, details.displayName(), currentQuota, currentOriginUsage, details.currentUsage(), details.expectedUsage()),
        Messages::WebPageProxy::ExceededDatabaseQuota::Reply(newQuota), m_page->pageID());

    DatabaseTracker::tracker().setQuota(origin, newQuota);
}
#endif


void WebChromeClient::reachedMaxAppCacheSize(int64_t)
{
    notImplemented();
}

void WebChromeClient::reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t)
{
    notImplemented();
}

#if ENABLE(DASHBOARD_SUPPORT)
void WebChromeClient::dashboardRegionsChanged()
{
    notImplemented();
}
#endif

void WebChromeClient::populateVisitedLinks()
{
}

FloatRect WebChromeClient::customHighlightRect(Node*, const AtomicString& type, const FloatRect& lineRect)
{
    notImplemented();
    return FloatRect();
}

void WebChromeClient::paintCustomHighlight(Node*, const AtomicString& type, const FloatRect& boxRect, const FloatRect& lineRect,
                                  bool behindText, bool entireLine)
{
    notImplemented();
}

bool WebChromeClient::shouldReplaceWithGeneratedFileForUpload(const String& path, String& generatedFilename)
{
    generatedFilename = m_page->injectedBundleUIClient().shouldGenerateFileForUpload(m_page, path);
    return !generatedFilename.isNull();
}

String WebChromeClient::generateReplacementFile(const String& path)
{
    return m_page->injectedBundleUIClient().generateFileForUpload(m_page, path);
}

bool WebChromeClient::paintCustomOverhangArea(GraphicsContext* context, const IntRect& horizontalOverhangArea, const IntRect& verticalOverhangArea, const IntRect& dirtyRect)
{
    if (!m_page->injectedBundleUIClient().shouldPaintCustomOverhangArea())
        return false;

    m_page->injectedBundleUIClient().paintCustomOverhangArea(m_page, context, horizontalOverhangArea, verticalOverhangArea, dirtyRect);
    return true;
}

void WebChromeClient::requestGeolocationPermissionForFrame(Frame*, Geolocation*)
{
    notImplemented();
}

void WebChromeClient::cancelGeolocationPermissionRequestForFrame(Frame*, Geolocation*)
{
    notImplemented();
}

void WebChromeClient::runOpenPanel(Frame* frame, PassRefPtr<FileChooser> prpFileChooser)
{
    if (m_page->activeOpenPanelResultListener())
        return;

    RefPtr<FileChooser> fileChooser = prpFileChooser;

    m_page->setActiveOpenPanelResultListener(WebOpenPanelResultListener::create(m_page, fileChooser.get()));
    m_page->send(Messages::WebPageProxy::RunOpenPanel(static_cast<WebFrameLoaderClient*>(frame->loader()->client())->webFrame()->frameID(), fileChooser->settings()));
}

void WebChromeClient::loadIconForFiles(const Vector<String>& filenames, FileIconLoader* loader)
{
    loader->notifyFinished(Icon::createIconForFiles(filenames));
}

void WebChromeClient::setCursor(const WebCore::Cursor& cursor)
{
#if USE(LAZY_NATIVE_CURSOR)
    m_page->send(Messages::WebPageProxy::SetCursor(cursor));
#endif
}

void WebChromeClient::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    m_page->send(Messages::WebPageProxy::SetCursorHiddenUntilMouseMoves(hiddenUntilMouseMoves));
}

void WebChromeClient::formStateDidChange(const Node*)
{
    notImplemented();
}

bool WebChromeClient::selectItemWritingDirectionIsNatural()
{
#if PLATFORM(WIN)
    return true;
#else
    return false;
#endif
}

bool WebChromeClient::selectItemAlignmentFollowsMenuWritingDirection()
{
#if PLATFORM(WIN)
    return false;
#else
    return true;
#endif
}

PassRefPtr<WebCore::PopupMenu> WebChromeClient::createPopupMenu(WebCore::PopupMenuClient* client) const
{
    return WebPopupMenu::create(m_page, client);
}

PassRefPtr<WebCore::SearchPopupMenu> WebChromeClient::createSearchPopupMenu(WebCore::PopupMenuClient* client) const
{
    return WebSearchPopupMenu::create(m_page, client);
}

#if ENABLE(CONTEXT_MENUS)
void WebChromeClient::showContextMenu()
{
    m_page->contextMenu()->show();
}
#endif

#if USE(ACCELERATED_COMPOSITING)
void WebChromeClient::attachRootGraphicsLayer(Frame*, GraphicsLayer* layer)
{
    if (layer)
        m_page->enterAcceleratedCompositingMode(layer);
    else
        m_page->exitAcceleratedCompositingMode();
}

void WebChromeClient::setNeedsOneShotDrawingSynchronization()
{
    notImplemented();
}

void WebChromeClient::scheduleCompositingLayerSync()
{
    if (m_page->drawingArea())
        m_page->drawingArea()->scheduleCompositingLayerSync();
}

#endif

#if ENABLE(TOUCH_EVENTS)
void WebChromeClient::needTouchEvents(bool needTouchEvents)
{
    m_page->send(Messages::WebPageProxy::NeedTouchEvents(needTouchEvents));
}
#endif

#if PLATFORM(WIN)
void WebChromeClient::setLastSetCursorToCurrentCursor()
{
}
#endif

#if ENABLE(FULLSCREEN_API)
bool WebChromeClient::supportsFullScreenForElement(const WebCore::Element* element, bool withKeyboard)
{
    return m_page->fullScreenManager()->supportsFullScreen(withKeyboard);
}

void WebChromeClient::enterFullScreenForElement(WebCore::Element* element)
{
    m_page->fullScreenManager()->enterFullScreenForElement(element);
}

void WebChromeClient::exitFullScreenForElement(WebCore::Element* element)
{
    m_page->fullScreenManager()->exitFullScreenForElement(element);
}
    
void WebChromeClient::setRootFullScreenLayer(GraphicsLayer* layer)
{
    m_page->fullScreenManager()->setRootFullScreenLayer(layer);
}

#endif

void WebChromeClient::dispatchViewportPropertiesDidChange(const ViewportArguments& args) const
{
    m_page->send(Messages::WebPageProxy::DidChangeViewportProperties(args));

#if USE(TILED_BACKING_STORE)
    // When viewport properties change, recalculate and set the new recommended layout size in case of fixed layout rendering.
    if (m_page->useFixedLayout()) {
        Settings* settings = m_page->corePage()->settings();

        int minimumLayoutFallbackWidth = std::max(settings->layoutFallbackWidth(), m_page->viewportSize().width());

        IntSize targetLayoutSize = computeViewportAttributes(m_page->corePage()->viewportArguments(),
            minimumLayoutFallbackWidth, settings->deviceWidth(), settings->deviceHeight(),
            settings->deviceDPI(), m_page->viewportSize()).layoutSize;
        m_page->setResizesToContentsUsingLayoutSize(targetLayoutSize);
    }
#endif
}

void WebChromeClient::didStartRubberBandForFrame(Frame*, const IntSize&) const
{
    m_page->drawingArea()->disableDisplayThrottling();
}

void WebChromeClient::didCompleteRubberBandForFrame(Frame* frame, const IntSize& initialOverhang) const
{
    m_page->drawingArea()->enableDisplayThrottling();
}

void WebChromeClient::didStartAnimatedScroll() const
{
    m_page->drawingArea()->disableDisplayThrottling();
}

void WebChromeClient::didCompleteAnimatedScroll() const
{
    m_page->drawingArea()->enableDisplayThrottling();
}
    
void WebChromeClient::notifyScrollerThumbIsVisibleInRect(const IntRect& scrollerThumb)
{
    m_page->send(Messages::WebPageProxy::NotifyScrollerThumbIsVisibleInRect(scrollerThumb));
}

void WebChromeClient::recommendedScrollbarStyleDidChange(int32_t newStyle)
{
    m_page->send(Messages::WebPageProxy::RecommendedScrollbarStyleDidChange(newStyle));
}

bool WebChromeClient::shouldRubberBandInDirection(WebCore::ScrollDirection direction) const
{
    ASSERT(direction != WebCore::ScrollUp && direction != WebCore::ScrollDown);
    
    if (direction == WebCore::ScrollLeft)
        return m_page->injectedBundleUIClient().shouldRubberBandInDirection(m_page, WKScrollDirectionLeft);
    if (direction == WebCore::ScrollRight)
        return m_page->injectedBundleUIClient().shouldRubberBandInDirection(m_page, WKScrollDirectionRight);

    ASSERT_NOT_REACHED();
    return true;
}

void WebChromeClient::numWheelEventHandlersChanged(unsigned count)
{
    m_page->send(Messages::WebPageProxy::NumWheelEventHandlersChanged(count));
}

} // namespace WebKit
