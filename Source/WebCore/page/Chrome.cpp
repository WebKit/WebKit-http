/*
 * Copyright (C) 2006, 2007, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Chrome.h"

#include "ChromeClient.h"
#include "DNS.h"
#include "Document.h"
#include "FileIconLoader.h"
#include "FileChooser.h"
#include "FileList.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameTree.h"
#include "Geolocation.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "Icon.h"
#include "InspectorInstrumentation.h"
#include "Page.h"
#include "PageGroupLoadDeferrer.h"
#include "RenderObject.h"
#include "ResourceHandle.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "StorageNamespace.h"
#include "WindowFeatures.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(INPUT_COLOR)
#include "ColorChooser.h"
#endif

namespace WebCore {

using namespace HTMLNames;
using namespace std;

Chrome::Chrome(Page* page, ChromeClient* client)
    : m_page(page)
    , m_client(client)
{
    ASSERT(m_client);
}

Chrome::~Chrome()
{
    m_client->chromeDestroyed();
}

void Chrome::invalidateRootView(const IntRect& updateRect, bool immediate)
{
    m_client->invalidateRootView(updateRect, immediate);
}

void Chrome::invalidateContentsAndRootView(const IntRect& updateRect, bool immediate)
{
    m_client->invalidateContentsAndRootView(updateRect, immediate);
}

void Chrome::invalidateContentsForSlowScroll(const IntRect& updateRect, bool immediate)
{
    m_client->invalidateContentsForSlowScroll(updateRect, immediate);
}

void Chrome::scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect)
{
    m_client->scroll(scrollDelta, rectToScroll, clipRect);
}

#if USE(TILED_BACKING_STORE)
void Chrome::delegatedScrollRequested(const IntPoint& scrollPoint)
{
    m_client->delegatedScrollRequested(scrollPoint);
}
#endif

IntPoint Chrome::screenToRootView(const IntPoint& point) const
{
    return m_client->screenToRootView(point);
}

IntRect Chrome::rootViewToScreen(const IntRect& rect) const
{
    return m_client->rootViewToScreen(rect);
}

PlatformPageClient Chrome::platformPageClient() const
{
    return m_client->platformPageClient();
}

void Chrome::contentsSizeChanged(Frame* frame, const IntSize& size) const
{
    m_client->contentsSizeChanged(frame, size);
}

void Chrome::layoutUpdated(Frame* frame) const
{
    m_client->layoutUpdated(frame);
}

void Chrome::scrollRectIntoView(const IntRect& rect) const
{
    m_client->scrollRectIntoView(rect);
}

void Chrome::scrollbarsModeDidChange() const
{
    m_client->scrollbarsModeDidChange();
}

void Chrome::setWindowRect(const FloatRect& rect) const
{
    m_client->setWindowRect(rect);
}

FloatRect Chrome::windowRect() const
{
    return m_client->windowRect();
}

FloatRect Chrome::pageRect() const
{
    return m_client->pageRect();
}

void Chrome::focus() const
{
    m_client->focus();
}

void Chrome::unfocus() const
{
    m_client->unfocus();
}

bool Chrome::canTakeFocus(FocusDirection direction) const
{
    return m_client->canTakeFocus(direction);
}

void Chrome::takeFocus(FocusDirection direction) const
{
    m_client->takeFocus(direction);
}

void Chrome::focusedNodeChanged(Node* node) const
{
    m_client->focusedNodeChanged(node);
}

void Chrome::focusedFrameChanged(Frame* frame) const
{
    m_client->focusedFrameChanged(frame);
}

Page* Chrome::createWindow(Frame* frame, const FrameLoadRequest& request, const WindowFeatures& features, const NavigationAction& action) const
{
    Page* newPage = m_client->createWindow(frame, request, features, action);

    if (newPage) {
        if (StorageNamespace* oldSessionStorage = m_page->sessionStorage(false))
            newPage->setSessionStorage(oldSessionStorage->copy());
    }

    return newPage;
}

void Chrome::show() const
{
    m_client->show();
}

bool Chrome::canRunModal() const
{
    return m_client->canRunModal();
}

static bool canRunModalIfDuringPageDismissal(Page* page, ChromeClient::DialogType dialog, const String& message)
{
    for (Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameLoader::PageDismissalType dismissal = frame->loader()->pageDismissalEventBeingDispatched();
        if (dismissal != FrameLoader::NoDismissal)
            return page->chrome()->client()->shouldRunModalDialogDuringPageDismissal(dialog, message, dismissal);
    }
    return true;
}

bool Chrome::canRunModalNow() const
{
    // If loads are blocked, we can't run modal because the contents
    // of the modal dialog will never show up!
    return canRunModal() && !ResourceHandle::loadsBlocked()
           && canRunModalIfDuringPageDismissal(m_page, ChromeClient::HTMLDialog, String());
}

void Chrome::runModal() const
{
    // Defer callbacks in all the other pages in this group, so we don't try to run JavaScript
    // in a way that could interact with this view.
    PageGroupLoadDeferrer deferrer(m_page, false);

    TimerBase::fireTimersInNestedEventLoop();
    m_client->runModal();
}

void Chrome::setToolbarsVisible(bool b) const
{
    m_client->setToolbarsVisible(b);
}

bool Chrome::toolbarsVisible() const
{
    return m_client->toolbarsVisible();
}

void Chrome::setStatusbarVisible(bool b) const
{
    m_client->setStatusbarVisible(b);
}

bool Chrome::statusbarVisible() const
{
    return m_client->statusbarVisible();
}

void Chrome::setScrollbarsVisible(bool b) const
{
    m_client->setScrollbarsVisible(b);
}

bool Chrome::scrollbarsVisible() const
{
    return m_client->scrollbarsVisible();
}

void Chrome::setMenubarVisible(bool b) const
{
    m_client->setMenubarVisible(b);
}

bool Chrome::menubarVisible() const
{
    return m_client->menubarVisible();
}

void Chrome::setResizable(bool b) const
{
    m_client->setResizable(b);
}

bool Chrome::canRunBeforeUnloadConfirmPanel()
{
    return m_client->canRunBeforeUnloadConfirmPanel();
}

bool Chrome::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    // Defer loads in case the client method runs a new event loop that would
    // otherwise cause the load to continue while we're in the middle of executing JavaScript.
    PageGroupLoadDeferrer deferrer(m_page, true);

    return m_client->runBeforeUnloadConfirmPanel(message, frame);
}

void Chrome::closeWindowSoon()
{
    m_client->closeWindowSoon();
}

void Chrome::runJavaScriptAlert(Frame* frame, const String& message)
{
    if (!canRunModalIfDuringPageDismissal(m_page, ChromeClient::AlertDialog, message))
        return;

    // Defer loads in case the client method runs a new event loop that would
    // otherwise cause the load to continue while we're in the middle of executing JavaScript.
    PageGroupLoadDeferrer deferrer(m_page, true);

    ASSERT(frame);
    m_client->runJavaScriptAlert(frame, frame->displayStringModifiedByEncoding(message));
}

bool Chrome::runJavaScriptConfirm(Frame* frame, const String& message)
{
    if (!canRunModalIfDuringPageDismissal(m_page, ChromeClient::ConfirmDialog, message))
        return false;

    // Defer loads in case the client method runs a new event loop that would
    // otherwise cause the load to continue while we're in the middle of executing JavaScript.
    PageGroupLoadDeferrer deferrer(m_page, true);

    ASSERT(frame);
    return m_client->runJavaScriptConfirm(frame, frame->displayStringModifiedByEncoding(message));
}

bool Chrome::runJavaScriptPrompt(Frame* frame, const String& prompt, const String& defaultValue, String& result)
{
    if (!canRunModalIfDuringPageDismissal(m_page, ChromeClient::PromptDialog, prompt))
        return false;

    // Defer loads in case the client method runs a new event loop that would
    // otherwise cause the load to continue while we're in the middle of executing JavaScript.
    PageGroupLoadDeferrer deferrer(m_page, true);

    ASSERT(frame);
    bool ok = m_client->runJavaScriptPrompt(frame, frame->displayStringModifiedByEncoding(prompt), frame->displayStringModifiedByEncoding(defaultValue), result);

    if (ok)
        result = frame->displayStringModifiedByEncoding(result);

    return ok;
}

void Chrome::setStatusbarText(Frame* frame, const String& status)
{
    ASSERT(frame);
    m_client->setStatusbarText(frame->displayStringModifiedByEncoding(status));
}

bool Chrome::shouldInterruptJavaScript()
{
    // Defer loads in case the client method runs a new event loop that would
    // otherwise cause the load to continue while we're in the middle of executing JavaScript.
    PageGroupLoadDeferrer deferrer(m_page, true);

    return m_client->shouldInterruptJavaScript();
}

#if ENABLE(REGISTER_PROTOCOL_HANDLER)
void Chrome::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title) 
{
    m_client->registerProtocolHandler(scheme, baseURL, url, title);
}
#endif

IntRect Chrome::windowResizerRect() const
{
    return m_client->windowResizerRect();
}

void Chrome::mouseDidMoveOverElement(const HitTestResult& result, unsigned modifierFlags)
{
    if (result.innerNode()) {
        Document* document = result.innerNode()->document();
        if (document && document->isDNSPrefetchEnabled())
            ResourceHandle::prepareForURL(result.absoluteLinkURL());
    }
    m_client->mouseDidMoveOverElement(result, modifierFlags);

    InspectorInstrumentation::mouseDidMoveOverElement(m_page, result, modifierFlags);
}

void Chrome::setToolTip(const HitTestResult& result)
{
    // First priority is a potential toolTip representing a spelling or grammar error
    TextDirection toolTipDirection;
    String toolTip = result.spellingToolTip(toolTipDirection);

    // Next priority is a toolTip from a URL beneath the mouse (if preference is set to show those).
    if (toolTip.isEmpty() && m_page->settings()->showsURLsInToolTips()) {
        if (Node* node = result.innerNonSharedNode()) {
            // Get tooltip representing form action, if relevant
            if (node->hasTagName(inputTag)) {
                HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
                if (input->isSubmitButton())
                    if (HTMLFormElement* form = input->form()) {
                        toolTip = form->action();
                        if (form->renderer())
                            toolTipDirection = form->renderer()->style()->direction();
                        else
                            toolTipDirection = LTR;
                    }
            }
        }

        // Get tooltip representing link's URL
        if (toolTip.isEmpty()) {
            // FIXME: Need to pass this URL through userVisibleString once that's in WebCore
            toolTip = result.absoluteLinkURL().string();
            // URL always display as LTR.
            toolTipDirection = LTR;
        }
    }

    // Next we'll consider a tooltip for element with "title" attribute
    if (toolTip.isEmpty())
        toolTip = result.title(toolTipDirection);

    if (toolTip.isEmpty() && m_page->settings()->showsToolTipOverTruncatedText())
        toolTip = result.innerTextIfTruncated(toolTipDirection);

    // Lastly, for <input type="file"> that allow multiple files, we'll consider a tooltip for the selected filenames
    if (toolTip.isEmpty()) {
        if (Node* node = result.innerNonSharedNode()) {
            if (node->hasTagName(inputTag)) {
                HTMLInputElement* input = static_cast<HTMLInputElement*>(node);
                toolTip = input->defaultToolTip();

                // FIXME: We should obtain text direction of tooltip from
                // ChromeClient or platform. As of October 2011, all client
                // implementations don't use text direction information for
                // ChromeClient::setToolTip. We'll work on tooltip text
                // direction during bidi cleanup in form inputs.
                toolTipDirection = LTR;
            }
        }
    }

    m_client->setToolTip(toolTip, toolTipDirection);
}

void Chrome::print(Frame* frame)
{
    // FIXME: This should have PageGroupLoadDeferrer, like runModal() or runJavaScriptAlert(), becasue it's no different from those.
    m_client->print(frame);
}

void Chrome::requestGeolocationPermissionForFrame(Frame* frame, Geolocation* geolocation)
{
    m_client->requestGeolocationPermissionForFrame(frame, geolocation);
}

void Chrome::cancelGeolocationPermissionRequestForFrame(Frame* frame, Geolocation* geolocation)
{
    m_client->cancelGeolocationPermissionRequestForFrame(frame, geolocation);
}

#if ENABLE(DIRECTORY_UPLOAD)
void Chrome::enumerateChosenDirectory(FileChooser* fileChooser)
{
    m_client->enumerateChosenDirectory(fileChooser);
}
#endif

#if ENABLE(INPUT_COLOR)
void Chrome::openColorChooser(ColorChooser* colorChooser, const Color& initialColor)
{
    m_client->openColorChooser(colorChooser, initialColor);
}

void Chrome::cleanupColorChooser(ColorChooser* colorChooser)
{
    m_client->cleanupColorChooser(colorChooser);
}

void Chrome::setSelectedColorInColorChooser(ColorChooser* colorChooser, const Color& color)
{
    m_client->setSelectedColorInColorChooser(colorChooser, color);
}
#endif

void Chrome::runOpenPanel(Frame* frame, PassRefPtr<FileChooser> fileChooser)
{
    m_client->runOpenPanel(frame, fileChooser);
}

void Chrome::loadIconForFiles(const Vector<String>& filenames, FileIconLoader* loader)
{
    m_client->loadIconForFiles(filenames, loader);
}

void Chrome::dispatchViewportPropertiesDidChange(const ViewportArguments& arguments) const
{
    m_client->dispatchViewportPropertiesDidChange(arguments);
}

void Chrome::setCursor(const Cursor& cursor)
{
    m_client->setCursor(cursor);
}

void Chrome::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    m_client->setCursorHiddenUntilMouseMoves(hiddenUntilMouseMoves);
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void Chrome::scheduleAnimation()
{
#if !USE(REQUEST_ANIMATION_FRAME_TIMER)
    m_client->scheduleAnimation();
#endif
}
#endif

#if ENABLE(NOTIFICATIONS)
NotificationPresenter* Chrome::notificationPresenter() const
{
    return m_client->notificationPresenter();
}
#endif

// --------

#if ENABLE(DASHBOARD_SUPPORT)
void ChromeClient::dashboardRegionsChanged()
{
}
#endif

void ChromeClient::populateVisitedLinks()
{
}

FloatRect ChromeClient::customHighlightRect(Node*, const AtomicString&, const FloatRect&)
{
    return FloatRect();
}

void ChromeClient::paintCustomHighlight(Node*, const AtomicString&, const FloatRect&, const FloatRect&, bool, bool)
{
}

bool ChromeClient::shouldReplaceWithGeneratedFileForUpload(const String&, String&)
{
    return false;
}

String ChromeClient::generateReplacementFile(const String&)
{
    ASSERT_NOT_REACHED();
    return String();
}

bool ChromeClient::paintCustomOverhangArea(GraphicsContext*, const IntRect&, const IntRect&, const IntRect&)
{
    return false;
}

bool Chrome::selectItemWritingDirectionIsNatural()
{
    return m_client->selectItemWritingDirectionIsNatural();
}

bool Chrome::selectItemAlignmentFollowsMenuWritingDirection()
{
    return m_client->selectItemAlignmentFollowsMenuWritingDirection();
}

PassRefPtr<PopupMenu> Chrome::createPopupMenu(PopupMenuClient* client) const
{
    return m_client->createPopupMenu(client);
}

PassRefPtr<SearchPopupMenu> Chrome::createSearchPopupMenu(PopupMenuClient* client) const
{
    return m_client->createSearchPopupMenu(client);
}

#if ENABLE(CONTEXT_MENUS)
void Chrome::showContextMenu()
{
    m_client->showContextMenu();
}
#endif

bool Chrome::requiresFullscreenForVideoPlayback()
{
    return m_client->requiresFullscreenForVideoPlayback();
}

} // namespace WebCore
