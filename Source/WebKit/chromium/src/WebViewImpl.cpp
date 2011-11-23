/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"
#include "WebViewImpl.h"

#include "AutofillPopupMenuClient.h"
#include "AXObjectCache.h"
#include "BackForwardListChromium.h"
#include "CSSStyleSelector.h"
#include "CSSValueKeywords.h"
#include "Chrome.h"
#include "ColorSpace.h"
#include "CompositionUnderlineVectorBuilder.h"
#include "ContextMenu.h"
#include "ContextMenuController.h"
#include "ContextMenuItem.h"
#include "Cursor.h"
#include "DOMUtilitiesPrivate.h"
#include "DeviceOrientationClientProxy.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DragController.h"
#include "DragData.h"
#include "DragScrollTimer.h"
#include "DragSession.h"
#include "Editor.h"
#include "EventHandler.h"
#include "Extensions3D.h"
#include "FocusController.h"
#include "FontDescription.h"
#include "FrameLoader.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "GeolocationClientProxy.h"
#include "GraphicsContext.h"
#include "GraphicsContext3D.h"
#include "GraphicsContext3DPrivate.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLTextAreaElement.h"
#include "HitTestResult.h"
#include "Image.h"
#include "ImageBuffer.h"
#include "InspectorController.h"
#include "KeyboardCodes.h"
#include "KeyboardEvent.h"
#include "LayerChromium.h"
#include "LayerPainterChromium.h"
#include "MIMETypeRegistry.h"
#include "NodeRenderStyle.h"
#include "NonCompositedContentHost.h"
#include "Page.h"
#include "PageGroup.h"
#include "PageGroupLoadDeferrer.h"
#include "Pasteboard.h"
#include "PlatformContextSkia.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformScreen.h"
#include "PlatformThemeChromiumLinux.h"
#include "PlatformWheelEvent.h"
#include "PopupContainer.h"
#include "PopupMenuClient.h"
#include "ProgressTracker.h"
#include "RenderLayerCompositor.h"
#include "RenderView.h"
#include "ResourceHandle.h"
#include "SchemeRegistry.h"
#include "ScrollAnimator.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "Settings.h"
#include "SpeechInputClientImpl.h"
#include "TextIterator.h"
#include "Timer.h"
#include "TraceEvent.h"
#include "TypingCommand.h"
#include "UserGestureIndicator.h"
#include "Vector.h"
#include "WebAccessibilityObject.h"
#include "WebAutofillClient.h"
#include "WebCompositorImpl.h"
#include "WebDevToolsAgentImpl.h"
#include "WebDevToolsAgentPrivate.h"
#include "WebDragData.h"
#include "WebFrameImpl.h"
#include "WebGraphicsContext3D.h"
#include "WebImage.h"
#include "WebInputElement.h"
#include "WebInputEvent.h"
#include "WebInputEventConversion.h"
#include "WebKit.h"
#include "WebKitPlatformSupport.h"
#include "WebMediaPlayerAction.h"
#include "WebNode.h"
#include "WebPlugin.h"
#include "WebPluginContainerImpl.h"
#include "WebPoint.h"
#include "WebPopupMenuImpl.h"
#include "WebRange.h"
#include "WebRect.h"
#include "WebRuntimeFeatures.h"
#include "WebSettingsImpl.h"
#include "WebString.h"
#include "WebVector.h"
#include "WebViewClient.h"
#include "cc/CCHeadsUpDisplay.h"
#include <wtf/ByteArray.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/RefPtr.h>

#if ENABLE(GESTURE_EVENTS)
#include "PlatformGestureEvent.h"
#endif

#if ENABLE(GESTURE_RECOGNIZER)
#include "PlatformGestureRecognizer.h"
#endif

#if USE(CG)
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>
#endif

#if OS(WINDOWS)
#include "RenderThemeChromiumWin.h"
#else
#if OS(UNIX) && !OS(DARWIN)
#include "RenderThemeChromiumLinux.h"
#endif
#include "RenderTheme.h"
#endif

// Get rid of WTF's pow define so we can use std::pow.
#undef pow
#include <cmath> // for std::pow

using namespace WebCore;
using namespace std;

namespace {

GraphicsContext3D::Attributes getCompositorContextAttributes()
{
    // Explicitly disable antialiasing for the compositor. As of the time of
    // this writing, the only platform that supported antialiasing for the
    // compositor was Mac OS X, because the on-screen OpenGL context creation
    // code paths on Windows and Linux didn't yet have multisampling support.
    // Mac OS X essentially always behaves as though it's rendering offscreen.
    // Multisampling has a heavy cost especially on devices with relatively low
    // fill rate like most notebooks, and the Mac implementation would need to
    // be optimized to resolve directly into the IOSurface shared between the
    // GPU and browser processes. For these reasons and to avoid platform
    // disparities we explicitly disable antialiasing.
    GraphicsContext3D::Attributes attributes;
    attributes.antialias = false;
    attributes.shareResources = true;
    return attributes;
}

} // anonymous namespace

namespace WebKit {

// Change the text zoom level by kTextSizeMultiplierRatio each time the user
// zooms text in or out (ie., change by 20%).  The min and max values limit
// text zoom to half and 3x the original text size.  These three values match
// those in Apple's port in WebKit/WebKit/WebView/WebView.mm
const double WebView::textSizeMultiplierRatio = 1.2;
const double WebView::minTextSizeMultiplier = 0.5;
const double WebView::maxTextSizeMultiplier = 3.0;
const float WebView::minPageScaleFactor = 0.25;
const float WebView::maxPageScaleFactor = 4.0;


// The group name identifies a namespace of pages.  Page group is used on OSX
// for some programs that use HTML views to display things that don't seem like
// web pages to the user (so shouldn't have visited link coloring).  We only use
// one page group.
const char* pageGroupName = "default";

// Used to defer all page activity in cases where the embedder wishes to run
// a nested event loop. Using a stack enables nesting of message loop invocations.
static Vector<PageGroupLoadDeferrer*>& pageGroupLoadDeferrerStack()
{
    DEFINE_STATIC_LOCAL(Vector<PageGroupLoadDeferrer*>, deferrerStack, ());
    return deferrerStack;
}

// Ensure that the WebDragOperation enum values stay in sync with the original
// DragOperation constants.
#define COMPILE_ASSERT_MATCHING_ENUM(coreName) \
    COMPILE_ASSERT(int(coreName) == int(Web##coreName), dummy##coreName)
COMPILE_ASSERT_MATCHING_ENUM(DragOperationNone);
COMPILE_ASSERT_MATCHING_ENUM(DragOperationCopy);
COMPILE_ASSERT_MATCHING_ENUM(DragOperationLink);
COMPILE_ASSERT_MATCHING_ENUM(DragOperationGeneric);
COMPILE_ASSERT_MATCHING_ENUM(DragOperationPrivate);
COMPILE_ASSERT_MATCHING_ENUM(DragOperationMove);
COMPILE_ASSERT_MATCHING_ENUM(DragOperationDelete);
COMPILE_ASSERT_MATCHING_ENUM(DragOperationEvery);

static const PopupContainerSettings autofillPopupSettings = {
    false, // setTextOnIndexChange
    false, // acceptOnAbandon
    true, // loopSelectionNavigation
    false // restrictWidthOfListBox (For security reasons show the entire entry
          // so the user doesn't enter information he did not intend to.)
};

static bool shouldUseExternalPopupMenus = false;

// WebView ----------------------------------------------------------------

WebView* WebView::create(WebViewClient* client)
{
    // Keep runtime flag for device motion turned off until it's implemented.
    WebRuntimeFeatures::enableDeviceMotion(false);

    // Pass the WebViewImpl's self-reference to the caller.
    return adoptRef(new WebViewImpl(client)).leakRef();
}

void WebView::setUseExternalPopupMenus(bool useExternalPopupMenus)
{
    shouldUseExternalPopupMenus = useExternalPopupMenus;
}

void WebView::updateVisitedLinkState(unsigned long long linkHash)
{
    Page::visitedStateChanged(PageGroup::pageGroup(pageGroupName), linkHash);
}

void WebView::resetVisitedLinkState()
{
    Page::allVisitedStateChanged(PageGroup::pageGroup(pageGroupName));
}

void WebView::willEnterModalLoop()
{
    PageGroup* pageGroup = PageGroup::pageGroup(pageGroupName);
    ASSERT(pageGroup);

    if (pageGroup->pages().isEmpty())
        pageGroupLoadDeferrerStack().append(static_cast<PageGroupLoadDeferrer*>(0));
    else {
        // Pick any page in the page group since we are deferring all pages.
        pageGroupLoadDeferrerStack().append(new PageGroupLoadDeferrer(*pageGroup->pages().begin(), true));
    }
}

void WebView::didExitModalLoop()
{
    ASSERT(pageGroupLoadDeferrerStack().size());

    delete pageGroupLoadDeferrerStack().last();
    pageGroupLoadDeferrerStack().removeLast();
}

void WebViewImpl::initializeMainFrame(WebFrameClient* frameClient)
{
    // NOTE: The WebFrameImpl takes a reference to itself within InitMainFrame
    // and releases that reference once the corresponding Frame is destroyed.
    RefPtr<WebFrameImpl> frame = WebFrameImpl::create(frameClient);

    frame->initializeAsMainFrame(this);

    // Restrict the access to the local file system
    // (see WebView.mm WebView::_commonInitializationWithFrameName).
    SecurityPolicy::setLocalLoadPolicy(SecurityPolicy::AllowLocalLoadsForLocalOnly);
}

void WebViewImpl::setAutofillClient(WebAutofillClient* autofillClient)
{
    m_autofillClient = autofillClient;
}

void WebViewImpl::setDevToolsAgentClient(WebDevToolsAgentClient* devToolsClient)
{
    if (devToolsClient)
        m_devToolsAgent = adoptPtr(new WebDevToolsAgentImpl(this, devToolsClient));
    else
        m_devToolsAgent.clear();
}

void WebViewImpl::setPermissionClient(WebPermissionClient* permissionClient)
{
    m_permissionClient = permissionClient;
}

void WebViewImpl::setSpellCheckClient(WebSpellCheckClient* spellCheckClient)
{
    m_spellCheckClient = spellCheckClient;
}

WebViewImpl::WebViewImpl(WebViewClient* client)
    : m_client(client)
    , m_autofillClient(0)
    , m_permissionClient(0)
    , m_spellCheckClient(0)
    , m_chromeClientImpl(this)
    , m_contextMenuClientImpl(this)
    , m_dragClientImpl(this)
    , m_editorClientImpl(this)
    , m_inspectorClientImpl(this)
    , m_observedNewNavigation(false)
#ifndef NDEBUG
    , m_newNavigationLoader(0)
#endif
    , m_zoomLevel(0)
    , m_minimumZoomLevel(zoomFactorToZoomLevel(minTextSizeMultiplier))
    , m_maximumZoomLevel(zoomFactorToZoomLevel(maxTextSizeMultiplier))
    , m_minimumPageScaleFactor(minPageScaleFactor)
    , m_maximumPageScaleFactor(maxPageScaleFactor)
    , m_contextMenuAllowed(false)
    , m_doingDragAndDrop(false)
    , m_ignoreInputEvents(false)
    , m_suppressNextKeypressEvent(false)
    , m_initialNavigationPolicy(WebNavigationPolicyIgnore)
    , m_imeAcceptEvents(true)
    , m_operationsAllowed(WebDragOperationNone)
    , m_dragOperation(WebDragOperationNone)
    , m_autofillPopupShowing(false)
    , m_autofillPopup(0)
    , m_isTransparent(false)
    , m_tabsToLinks(false)
    , m_dragScrollTimer(adoptPtr(new DragScrollTimer))
#if USE(ACCELERATED_COMPOSITING)
    , m_rootGraphicsLayer(0)
    , m_isAcceleratedCompositingActive(false)
    , m_compositorCreationFailed(false)
    , m_recreatingGraphicsContext(false)
#endif
#if ENABLE(INPUT_SPEECH)
    , m_speechInputClient(SpeechInputClientImpl::create(client))
#endif
    , m_deviceOrientationClientProxy(adoptPtr(new DeviceOrientationClientProxy(client ? client->deviceOrientationClient() : 0)))
    , m_geolocationClientProxy(adoptPtr(new GeolocationClientProxy(client ? client->geolocationClient() : 0)))
#if ENABLE(GESTURE_RECOGNIZER)
    , m_gestureRecognizer(WebCore::PlatformGestureRecognizer::create())
#endif
#if ENABLE(MEDIA_STREAM)
    , m_userMediaClientImpl(this)
#endif
{
    // WebKit/win/WebView.cpp does the same thing, except they call the
    // KJS specific wrapper around this method. We need to have threading
    // initialized because CollatorICU requires it.
    WTF::initializeThreading();
    WTF::initializeMainThread();

    // set to impossible point so we always get the first mouse pos
    m_lastMousePosition = WebPoint(-1, -1);

    Page::PageClients pageClients;
    pageClients.chromeClient = &m_chromeClientImpl;
    pageClients.contextMenuClient = &m_contextMenuClientImpl;
    pageClients.editorClient = &m_editorClientImpl;
    pageClients.dragClient = &m_dragClientImpl;
    pageClients.inspectorClient = &m_inspectorClientImpl;
#if ENABLE(INPUT_SPEECH)
    pageClients.speechInputClient = m_speechInputClient.get();
#endif
    pageClients.deviceOrientationClient = m_deviceOrientationClientProxy.get();
    pageClients.geolocationClient = m_geolocationClientProxy.get();
    pageClients.backForwardClient = BackForwardListChromium::create(this);
#if ENABLE(MEDIA_STREAM)
    pageClients.userMediaClient = &m_userMediaClientImpl;
#endif

    m_page = adoptPtr(new Page(pageClients));

    m_geolocationClientProxy->setController(m_page->geolocationController());

    m_page->setGroupName(pageGroupName);

#if ENABLE(PAGE_VISIBILITY_API)
    if (m_client)
        setVisibilityState(m_client->visibilityState(), true);
#endif

    m_inspectorSettingsMap = adoptPtr(new SettingsMap);
}

WebViewImpl::~WebViewImpl()
{
    ASSERT(!m_page);
}

RenderTheme* WebViewImpl::theme() const
{
    return m_page ? m_page->theme() : RenderTheme::defaultTheme().get();
}

WebFrameImpl* WebViewImpl::mainFrameImpl()
{
    return m_page ? WebFrameImpl::fromFrame(m_page->mainFrame()) : 0;
}

bool WebViewImpl::tabKeyCyclesThroughElements() const
{
    ASSERT(m_page);
    return m_page->tabKeyCyclesThroughElements();
}

void WebViewImpl::setTabKeyCyclesThroughElements(bool value)
{
    if (m_page)
        m_page->setTabKeyCyclesThroughElements(value);
}

void WebViewImpl::mouseMove(const WebMouseEvent& event)
{
    if (!mainFrameImpl() || !mainFrameImpl()->frameView())
        return;

    m_lastMousePosition = WebPoint(event.x, event.y);

    // We call mouseMoved here instead of handleMouseMovedEvent because we need
    // our ChromeClientImpl to receive changes to the mouse position and
    // tooltip text, and mouseMoved handles all of that.
    mainFrameImpl()->frame()->eventHandler()->mouseMoved(
        PlatformMouseEventBuilder(mainFrameImpl()->frameView(), event));
}

void WebViewImpl::mouseLeave(const WebMouseEvent& event)
{
    // This event gets sent as the main frame is closing.  In that case, just
    // ignore it.
    if (!mainFrameImpl() || !mainFrameImpl()->frameView())
        return;

    m_client->setMouseOverURL(WebURL());

    mainFrameImpl()->frame()->eventHandler()->handleMouseMoveEvent(
        PlatformMouseEventBuilder(mainFrameImpl()->frameView(), event));
}

void WebViewImpl::mouseDown(const WebMouseEvent& event)
{
    if (!mainFrameImpl() || !mainFrameImpl()->frameView())
        return;

    // If there is a select popup open, close it as the user is clicking on
    // the page (outside of the popup).  We also save it so we can prevent a
    // click on the select element from immediately reopening the popup.
    RefPtr<WebCore::PopupContainer> selectPopup;
    if (event.button == WebMouseEvent::ButtonLeft) {
        selectPopup = m_selectPopup;
        hideSelectPopup();
        ASSERT(!m_selectPopup);
    }

    m_lastMouseDownPoint = WebPoint(event.x, event.y);

    if (event.button == WebMouseEvent::ButtonLeft) {
        IntPoint point(event.x, event.y);
        point = m_page->mainFrame()->view()->windowToContents(point);
        HitTestResult result(m_page->mainFrame()->eventHandler()->hitTestResultAtPoint(point, false));
        Node* hitNode = result.innerNonSharedNode();

        // Take capture on a mouse down on a plugin so we can send it mouse events.
        if (hitNode && hitNode->renderer() && hitNode->renderer()->isEmbeddedObject())
            m_mouseCaptureNode = hitNode;
    }

    mainFrameImpl()->frame()->eventHandler()->handleMousePressEvent(
        PlatformMouseEventBuilder(mainFrameImpl()->frameView(), event));

    if (m_selectPopup && m_selectPopup == selectPopup) {
        // That click triggered a select popup which is the same as the one that
        // was showing before the click.  It means the user clicked the select
        // while the popup was showing, and as a result we first closed then
        // immediately reopened the select popup.  It needs to be closed.
        hideSelectPopup();
    }

    // Dispatch the contextmenu event regardless of if the click was swallowed.
    // On Windows, we handle it on mouse up, not down.
#if OS(DARWIN)
    if (event.button == WebMouseEvent::ButtonRight
        || (event.button == WebMouseEvent::ButtonLeft
            && event.modifiers & WebMouseEvent::ControlKey))
        mouseContextMenu(event);
#elif OS(UNIX)
    if (event.button == WebMouseEvent::ButtonRight)
        mouseContextMenu(event);
#endif
}

void WebViewImpl::mouseContextMenu(const WebMouseEvent& event)
{
    if (!mainFrameImpl() || !mainFrameImpl()->frameView())
        return;

    m_page->contextMenuController()->clearContextMenu();

    PlatformMouseEventBuilder pme(mainFrameImpl()->frameView(), event);

    // Find the right target frame. See issue 1186900.
    HitTestResult result = hitTestResultForWindowPos(pme.pos());
    Frame* targetFrame;
    if (result.innerNonSharedNode())
        targetFrame = result.innerNonSharedNode()->document()->frame();
    else
        targetFrame = m_page->focusController()->focusedOrMainFrame();

#if OS(WINDOWS)
    targetFrame->view()->setCursor(pointerCursor());
#endif

    m_contextMenuAllowed = true;
    targetFrame->eventHandler()->sendContextMenuEvent(pme);
    m_contextMenuAllowed = false;
    // Actually showing the context menu is handled by the ContextMenuClient
    // implementation...
}

void WebViewImpl::mouseUp(const WebMouseEvent& event)
{
    if (!mainFrameImpl() || !mainFrameImpl()->frameView())
        return;

#if OS(UNIX) && !OS(DARWIN)
    // If the event was a middle click, attempt to copy text into the focused
    // frame. We execute this before we let the page have a go at the event
    // because the page may change what is focused during in its event handler.
    //
    // This code is in the mouse up handler. There is some debate about putting
    // this here, as opposed to the mouse down handler.
    //   xterm: pastes on up.
    //   GTK: pastes on down.
    //   Firefox: pastes on up.
    //   Midori: couldn't paste at all with 0.1.2
    //
    // There is something of a webcompat angle to this well, as highlighted by
    // crbug.com/14608. Pages can clear text boxes 'onclick' and, if we paste on
    // down then the text is pasted just before the onclick handler runs and
    // clears the text box. So it's important this happens after the
    // handleMouseReleaseEvent() earlier in this function
    if (event.button == WebMouseEvent::ButtonMiddle) {
        Frame* focused = focusedWebCoreFrame();
        FrameView* view = m_page->mainFrame()->view();
        IntPoint clickPoint(m_lastMouseDownPoint.x, m_lastMouseDownPoint.y);
        IntPoint contentPoint = view->windowToContents(clickPoint);
        HitTestResult hitTestResult = focused->eventHandler()->hitTestResultAtPoint(contentPoint, false, false, ShouldHitTestScrollbars);
        // We don't want to send a paste when middle clicking a scroll bar or a
        // link (which will navigate later in the code).  The main scrollbars
        // have to be handled separately.
        if (!hitTestResult.scrollbar() && !hitTestResult.isLiveLink() && focused && !view->scrollbarAtPoint(clickPoint)) {
            Editor* editor = focused->editor();
            Pasteboard* pasteboard = Pasteboard::generalPasteboard();
            bool oldSelectionMode = pasteboard->isSelectionMode();
            pasteboard->setSelectionMode(true);
            editor->command(AtomicString("Paste")).execute();
            pasteboard->setSelectionMode(oldSelectionMode);
        }
    }
#endif

    mainFrameImpl()->frame()->eventHandler()->handleMouseReleaseEvent(
        PlatformMouseEventBuilder(mainFrameImpl()->frameView(), event));

#if OS(WINDOWS)
    // Dispatch the contextmenu event regardless of if the click was swallowed.
    // On Mac/Linux, we handle it on mouse down, not up.
    if (event.button == WebMouseEvent::ButtonRight)
        mouseContextMenu(event);
#endif
}

bool WebViewImpl::mouseWheel(const WebMouseWheelEvent& event)
{
    PlatformWheelEventBuilder platformEvent(mainFrameImpl()->frameView(), event);
    return mainFrameImpl()->frame()->eventHandler()->handleWheelEvent(platformEvent);
}

#if ENABLE(GESTURE_EVENTS)
bool WebViewImpl::gestureEvent(const WebGestureEvent& event)
{
    PlatformGestureEventBuilder platformEvent(mainFrameImpl()->frameView(), event);
    bool handled = mainFrameImpl()->frame()->eventHandler()->handleGestureEvent(platformEvent);

    Frame* frame = mainFrameImpl()->frame();
    WebPluginContainerImpl* pluginContainer = WebFrameImpl::pluginContainerFromFrame(frame);
    if (pluginContainer)
        handled |= pluginContainer->handleGestureEvent(platformEvent);

    return handled;
}
#endif

bool WebViewImpl::keyEvent(const WebKeyboardEvent& event)
{
    ASSERT((event.type == WebInputEvent::RawKeyDown)
        || (event.type == WebInputEvent::KeyDown)
        || (event.type == WebInputEvent::KeyUp));

    // Please refer to the comments explaining the m_suppressNextKeypressEvent
    // member.
    // The m_suppressNextKeypressEvent is set if the KeyDown is handled by
    // Webkit. A keyDown event is typically associated with a keyPress(char)
    // event and a keyUp event. We reset this flag here as this is a new keyDown
    // event.
    m_suppressNextKeypressEvent = false;

    // If there is a select popup, it should be the one processing the event,
    // not the page.
    if (m_selectPopup)
        return m_selectPopup->handleKeyEvent(PlatformKeyboardEventBuilder(event));

    // Give Autocomplete a chance to consume the key events it is interested in.
    if (autocompleteHandleKeyEvent(event))
        return true;

    RefPtr<Frame> frame = focusedWebCoreFrame();
    if (!frame)
        return false;

    EventHandler* handler = frame->eventHandler();
    if (!handler)
        return keyEventDefault(event);

#if !OS(DARWIN)
    const WebInputEvent::Type contextMenuTriggeringEventType =
#if OS(WINDOWS)
        WebInputEvent::KeyUp;
#elif OS(UNIX)
        WebInputEvent::RawKeyDown;
#endif

    bool isUnmodifiedMenuKey = !(event.modifiers & WebInputEvent::InputModifiers) && event.windowsKeyCode == VKEY_APPS;
    bool isShiftF10 = event.modifiers == WebInputEvent::ShiftKey && event.windowsKeyCode == VKEY_F10;
    if ((isUnmodifiedMenuKey || isShiftF10) && event.type == contextMenuTriggeringEventType) {
        sendContextMenuEvent(event);
        return true;
    }
#endif // !OS(DARWIN)

    PlatformKeyboardEventBuilder evt(event);

    if (handler->keyEvent(evt)) {
        if (WebInputEvent::RawKeyDown == event.type) {
            // Suppress the next keypress event unless the focused node is a plug-in node.
            // (Flash needs these keypress events to handle non-US keyboards.)
            Node* node = focusedWebCoreNode();
            if (!node || !node->renderer() || !node->renderer()->isEmbeddedObject())
                m_suppressNextKeypressEvent = true;
        }
        return true;
    }

    return keyEventDefault(event);
}

bool WebViewImpl::autocompleteHandleKeyEvent(const WebKeyboardEvent& event)
{
    if (!m_autofillPopupShowing
        // Home and End should be left to the text field to process.
        || event.windowsKeyCode == VKEY_HOME
        || event.windowsKeyCode == VKEY_END)
      return false;

    // Pressing delete triggers the removal of the selected suggestion from the DB.
    if (event.windowsKeyCode == VKEY_DELETE
        && m_autofillPopup->selectedIndex() != -1) {
        Node* node = focusedWebCoreNode();
        if (!node || (node->nodeType() != Node::ELEMENT_NODE)) {
            ASSERT_NOT_REACHED();
            return false;
        }
        Element* element = static_cast<Element*>(node);
        if (!element->hasLocalName(HTMLNames::inputTag)) {
            ASSERT_NOT_REACHED();
            return false;
        }

        int selectedIndex = m_autofillPopup->selectedIndex();

        if (!m_autofillPopupClient->canRemoveSuggestionAtIndex(selectedIndex))
            return false;

        WebString name = WebInputElement(static_cast<HTMLInputElement*>(element)).nameForAutofill();
        WebString value = m_autofillPopupClient->itemText(selectedIndex);
        m_autofillClient->removeAutocompleteSuggestion(name, value);
        // Update the entries in the currently showing popup to reflect the
        // deletion.
        m_autofillPopupClient->removeSuggestionAtIndex(selectedIndex);
        refreshAutofillPopup();
        return false;
    }

    if (!m_autofillPopup->isInterestedInEventForKey(event.windowsKeyCode))
        return false;

    if (m_autofillPopup->handleKeyEvent(PlatformKeyboardEventBuilder(event))) {
        // We need to ignore the next Char event after this otherwise pressing
        // enter when selecting an item in the menu will go to the page.
        if (WebInputEvent::RawKeyDown == event.type)
            m_suppressNextKeypressEvent = true;
        return true;
    }

    return false;
}

bool WebViewImpl::charEvent(const WebKeyboardEvent& event)
{
    ASSERT(event.type == WebInputEvent::Char);

    // Please refer to the comments explaining the m_suppressNextKeypressEvent
    // member.  The m_suppressNextKeypressEvent is set if the KeyDown is
    // handled by Webkit. A keyDown event is typically associated with a
    // keyPress(char) event and a keyUp event. We reset this flag here as it
    // only applies to the current keyPress event.
    bool suppress = m_suppressNextKeypressEvent;
    m_suppressNextKeypressEvent = false;

    // If there is a select popup, it should be the one processing the event,
    // not the page.
    if (m_selectPopup)
        return m_selectPopup->handleKeyEvent(PlatformKeyboardEventBuilder(event));

    Frame* frame = focusedWebCoreFrame();
    if (!frame)
        return suppress;

    EventHandler* handler = frame->eventHandler();
    if (!handler)
        return suppress || keyEventDefault(event);

    PlatformKeyboardEventBuilder evt(event);
    if (!evt.isCharacterKey())
        return true;

    // Accesskeys are triggered by char events and can't be suppressed.
    if (handler->handleAccessKey(evt))
        return true;

    // Safari 3.1 does not pass off windows system key messages (WM_SYSCHAR) to
    // the eventHandler::keyEvent. We mimic this behavior on all platforms since
    // for now we are converting other platform's key events to windows key
    // events.
    if (evt.isSystemKey())
        return false;

    if (!suppress && !handler->keyEvent(evt))
        return keyEventDefault(event);

    return true;
}

#if ENABLE(TOUCH_EVENTS)
bool WebViewImpl::touchEvent(const WebTouchEvent& event)
{
    if (!mainFrameImpl() || !mainFrameImpl()->frameView())
        return false;

    PlatformTouchEventBuilder touchEventBuilder(mainFrameImpl()->frameView(), event);
    bool defaultPrevented = mainFrameImpl()->frame()->eventHandler()->handleTouchEvent(touchEventBuilder);

#if ENABLE(GESTURE_RECOGNIZER)
    OwnPtr<Vector<WebCore::PlatformGestureEvent> > gestureEvents(m_gestureRecognizer->processTouchEventForGestures(touchEventBuilder, defaultPrevented));
    for (unsigned int  i = 0; i < gestureEvents->size(); i++)
        mainFrameImpl()->frame()->eventHandler()->handleGestureEvent(gestureEvents->at(i));
#endif

    return defaultPrevented;
}
#endif

void WebViewImpl::numberOfWheelEventHandlersChanged(unsigned numberOfWheelHandlers)
{
    m_haveWheelEventHandlers = numberOfWheelHandlers > 0;
    if (m_client)
        m_client->numberOfWheelEventHandlersChanged(numberOfWheelHandlers);
#if USE(ACCELERATED_COMPOSITING)
    if (m_layerTreeHost)
        m_layerTreeHost->setHaveWheelEventHandlers(m_haveWheelEventHandlers);
#endif
}

#if !OS(DARWIN)
// Mac has no way to open a context menu based on a keyboard event.
bool WebViewImpl::sendContextMenuEvent(const WebKeyboardEvent& event)
{
    // The contextMenuController() holds onto the last context menu that was
    // popped up on the page until a new one is created. We need to clear
    // this menu before propagating the event through the DOM so that we can
    // detect if we create a new menu for this event, since we won't create
    // a new menu if the DOM swallows the event and the defaultEventHandler does
    // not run.
    page()->contextMenuController()->clearContextMenu();

    m_contextMenuAllowed = true;
    Frame* focusedFrame = page()->focusController()->focusedOrMainFrame();
    bool handled = focusedFrame->eventHandler()->sendContextMenuEventForKey();
    m_contextMenuAllowed = false;
    return handled;
}
#endif

bool WebViewImpl::keyEventDefault(const WebKeyboardEvent& event)
{
    Frame* frame = focusedWebCoreFrame();
    if (!frame)
        return false;

    switch (event.type) {
    case WebInputEvent::Char:
        if (event.windowsKeyCode == VKEY_SPACE) {
            int keyCode = ((event.modifiers & WebInputEvent::ShiftKey) ? VKEY_PRIOR : VKEY_NEXT);
            return scrollViewWithKeyboard(keyCode, event.modifiers);
        }
        break;
    case WebInputEvent::RawKeyDown:
        if (event.modifiers == WebInputEvent::ControlKey) {
            switch (event.windowsKeyCode) {
#if !OS(DARWIN)
            case 'A':
                focusedFrame()->executeCommand(WebString::fromUTF8("SelectAll"));
                return true;
            case VKEY_INSERT:
            case 'C':
                focusedFrame()->executeCommand(WebString::fromUTF8("Copy"));
                return true;
#endif
            // Match FF behavior in the sense that Ctrl+home/end are the only Ctrl
            // key combinations which affect scrolling. Safari is buggy in the
            // sense that it scrolls the page for all Ctrl+scrolling key
            // combinations. For e.g. Ctrl+pgup/pgdn/up/down, etc.
            case VKEY_HOME:
            case VKEY_END:
                break;
            default:
                return false;
            }
        }
        if (!event.isSystemKey && !(event.modifiers & WebInputEvent::ShiftKey))
            return scrollViewWithKeyboard(event.windowsKeyCode, event.modifiers);
        break;
    default:
        break;
    }
    return false;
}

bool WebViewImpl::scrollViewWithKeyboard(int keyCode, int modifiers)
{
    ScrollDirection scrollDirection;
    ScrollGranularity scrollGranularity;
#if OS(DARWIN)
    // Control-Up/Down should be PageUp/Down on Mac.
    if (modifiers & WebMouseEvent::ControlKey) {
      if (keyCode == VKEY_UP)
        keyCode = VKEY_PRIOR;
      else if (keyCode == VKEY_DOWN)
        keyCode = VKEY_NEXT;
    }
#endif
    if (!mapKeyCodeForScroll(keyCode, &scrollDirection, &scrollGranularity))
        return false;
    return propagateScroll(scrollDirection, scrollGranularity);
}

bool WebViewImpl::mapKeyCodeForScroll(int keyCode,
                                      WebCore::ScrollDirection* scrollDirection,
                                      WebCore::ScrollGranularity* scrollGranularity)
{
    switch (keyCode) {
    case VKEY_LEFT:
        *scrollDirection = ScrollLeft;
        *scrollGranularity = ScrollByLine;
        break;
    case VKEY_RIGHT:
        *scrollDirection = ScrollRight;
        *scrollGranularity = ScrollByLine;
        break;
    case VKEY_UP:
        *scrollDirection = ScrollUp;
        *scrollGranularity = ScrollByLine;
        break;
    case VKEY_DOWN:
        *scrollDirection = ScrollDown;
        *scrollGranularity = ScrollByLine;
        break;
    case VKEY_HOME:
        *scrollDirection = ScrollUp;
        *scrollGranularity = ScrollByDocument;
        break;
    case VKEY_END:
        *scrollDirection = ScrollDown;
        *scrollGranularity = ScrollByDocument;
        break;
    case VKEY_PRIOR:  // page up
        *scrollDirection = ScrollUp;
        *scrollGranularity = ScrollByPage;
        break;
    case VKEY_NEXT:  // page down
        *scrollDirection = ScrollDown;
        *scrollGranularity = ScrollByPage;
        break;
    default:
        return false;
    }

    return true;
}

void WebViewImpl::hideSelectPopup()
{
    if (m_selectPopup)
        m_selectPopup->hidePopup();
}

bool WebViewImpl::propagateScroll(ScrollDirection scrollDirection,
                                  ScrollGranularity scrollGranularity)
{
    Frame* frame = focusedWebCoreFrame();
    if (!frame)
        return false;

    bool scrollHandled = frame->eventHandler()->scrollOverflow(scrollDirection, scrollGranularity);
    Frame* currentFrame = frame;
    while (!scrollHandled && currentFrame) {
        scrollHandled = currentFrame->view()->scroll(scrollDirection, scrollGranularity);
        currentFrame = currentFrame->tree()->parent();
    }
    return scrollHandled;
}

void  WebViewImpl::popupOpened(WebCore::PopupContainer* popupContainer)
{
    if (popupContainer->popupType() == WebCore::PopupContainer::Select) {
        ASSERT(!m_selectPopup);
        m_selectPopup = popupContainer;
    }
}

void  WebViewImpl::popupClosed(WebCore::PopupContainer* popupContainer)
{
    if (popupContainer->popupType() == WebCore::PopupContainer::Select) {
        ASSERT(m_selectPopup);
        m_selectPopup = 0;
    }
}

void WebViewImpl::hideAutofillPopup()
{
    if (m_autofillPopupShowing) {
        m_autofillPopup->hidePopup();
        m_autofillPopupShowing = false;
    }
}

Frame* WebViewImpl::focusedWebCoreFrame() const
{
    return m_page ? m_page->focusController()->focusedOrMainFrame() : 0;
}

WebViewImpl* WebViewImpl::fromPage(Page* page)
{
    if (!page)
        return 0;

    ChromeClientImpl* chromeClient = static_cast<ChromeClientImpl*>(page->chrome()->client());
    return static_cast<WebViewImpl*>(chromeClient->webView());
}

// WebWidget ------------------------------------------------------------------

void WebViewImpl::close()
{
    RefPtr<WebFrameImpl> mainFrameImpl;

    if (m_page) {
        // Initiate shutdown for the entire frameset.  This will cause a lot of
        // notifications to be sent.
        if (m_page->mainFrame()) {
            mainFrameImpl = WebFrameImpl::fromFrame(m_page->mainFrame());
            m_page->mainFrame()->loader()->frameDetached();
        }
        m_page.clear();
    }

    // Should happen after m_page.clear().
    if (m_devToolsAgent)
        m_devToolsAgent.clear();

    // Reset the delegate to prevent notifications being sent as we're being
    // deleted.
    m_client = 0;

    deref();  // Balances ref() acquired in WebView::create
}

void WebViewImpl::willStartLiveResize()
{
    if (mainFrameImpl() && mainFrameImpl()->frameView())
        mainFrameImpl()->frameView()->willStartLiveResize();

    Frame* frame = mainFrameImpl()->frame();
    WebPluginContainerImpl* pluginContainer = WebFrameImpl::pluginContainerFromFrame(frame);
    if (pluginContainer)
        pluginContainer->willStartLiveResize();
}

void WebViewImpl::resize(const WebSize& newSize)
{
    if (m_size == newSize)
        return;
    m_size = newSize;

    if (mainFrameImpl()->frameView()) {
        mainFrameImpl()->frameView()->resize(m_size.width, m_size.height);
        mainFrameImpl()->frame()->eventHandler()->sendResizeEvent();
    }

    if (m_client) {
        if (isAcceleratedCompositingActive()) {
#if USE(ACCELERATED_COMPOSITING)
            updateLayerTreeViewport();
#endif
        } else {
            WebRect damagedRect(0, 0, m_size.width, m_size.height);
            m_client->didInvalidateRect(damagedRect);
        }
    }
}

void WebViewImpl::willEndLiveResize()
{
    if (mainFrameImpl() && mainFrameImpl()->frameView())
        mainFrameImpl()->frameView()->willEndLiveResize();

    Frame* frame = mainFrameImpl()->frame();
    WebPluginContainerImpl* pluginContainer = WebFrameImpl::pluginContainerFromFrame(frame);
    if (pluginContainer)
        pluginContainer->willEndLiveResize();
}

void WebViewImpl::willEnterFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (!m_provisionalFullScreenElement)
        return;

    // Ensure that this element's document is still attached.
    Document* doc = m_provisionalFullScreenElement->document();
    if (doc->frame()) {
        doc->webkitWillEnterFullScreenForElement(m_provisionalFullScreenElement.get());
        m_fullScreenFrame = doc->frame();
    }
    m_provisionalFullScreenElement.clear();
#endif
}

void WebViewImpl::didEnterFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (!m_fullScreenFrame)
        return;

    if (Document* doc = m_fullScreenFrame->document()) {
        if (doc->webkitIsFullScreen())
            doc->webkitDidEnterFullScreenForElement(0);
    }
#endif
}

void WebViewImpl::willExitFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (!m_fullScreenFrame)
        return;

    if (Document* doc = m_fullScreenFrame->document()) {
        if (doc->webkitIsFullScreen())
            doc->webkitWillExitFullScreenForElement(0);
    }
#endif
}

void WebViewImpl::didExitFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (!m_fullScreenFrame)
        return;

    if (Document* doc = m_fullScreenFrame->document()) {
        if (doc->webkitIsFullScreen())
            doc->webkitDidExitFullScreenForElement(0);
    }

    m_fullScreenFrame.clear();
#endif
}

void WebViewImpl::animate(double frameBeginTime)
{
#if ENABLE(REQUEST_ANIMATION_FRAME)
    TRACE_EVENT("WebViewImpl::animate", this, 0);
    // FIXME: remove this zero-check once render_widget has been modified to
    // pass in a frameBeginTime.
    if (!frameBeginTime)
        frameBeginTime = currentTime();
    WebFrameImpl* webframe = mainFrameImpl();
    if (webframe) {
        FrameView* view = webframe->frameView();
        if (view) {
            if (!settings()->useThreadedCompositor() && m_layerTreeHost)
                m_layerTreeHost->setAnimating(true);

            view->serviceScriptedAnimations(convertSecondsToDOMTimeStamp(frameBeginTime));

            if (!settings()->useThreadedCompositor() && m_layerTreeHost)
                m_layerTreeHost->setAnimating(false);
        }
    }
#endif
}

void WebViewImpl::layout()
{
    TRACE_EVENT("WebViewImpl::layout", this, 0);

    WebFrameImpl* webframe = mainFrameImpl();
    if (webframe) {
        // In order for our child HWNDs (NativeWindowWidgets) to update properly,
        // they need to be told that we are updating the screen.  The problem is
        // that the native widgets need to recalculate their clip region and not
        // overlap any of our non-native widgets.  To force the resizing, call
        // setFrameRect().  This will be a quick operation for most frames, but
        // the NativeWindowWidgets will update a proper clipping region.
        FrameView* view = webframe->frameView();
        if (view)
            view->setFrameRect(view->frameRect());

        // setFrameRect may have the side-effect of causing existing page
        // layout to be invalidated, so layout needs to be called last.

        webframe->layout();
    }
}

#if USE(ACCELERATED_COMPOSITING)
void WebViewImpl::doPixelReadbackToCanvas(WebCanvas* canvas, const IntRect& rect)
{
#if USE(SKIA)
    PlatformContextSkia context(canvas);

    // PlatformGraphicsContext is actually a pointer to PlatformContextSkia
    GraphicsContext gc(reinterpret_cast<PlatformGraphicsContext*>(&context));
    int bitmapHeight = canvas->getDevice()->accessBitmap(false).height();
#elif USE(CG)
    GraphicsContext gc(canvas);
    int bitmapHeight = CGBitmapContextGetHeight(reinterpret_cast<CGContextRef>(canvas));
#else
    notImplemented();
#endif
    // Compute rect to sample from inverted GPU buffer.
    IntRect invertRect(rect.x(), bitmapHeight - rect.maxY(), rect.width(), rect.height());

    OwnPtr<ImageBuffer> imageBuffer(ImageBuffer::create(rect.size()));
    RefPtr<ByteArray> pixelArray(ByteArray::create(rect.width() * rect.height() * 4));
    if (imageBuffer && pixelArray) {
        m_layerTreeHost->compositeAndReadback(pixelArray->data(), invertRect);
        imageBuffer->putPremultipliedImageData(pixelArray.get(), rect.size(), IntRect(IntPoint(), rect.size()), IntPoint());
        gc.save();
        gc.translate(IntSize(0, bitmapHeight));
        gc.scale(FloatSize(1.0f, -1.0f));
        // Use invertRect in next line, so that transform above inverts it back to
        // desired destination rect.
        gc.drawImageBuffer(imageBuffer.get(), ColorSpaceDeviceRGB, invertRect.location());
        gc.restore();
    }
}
#endif

void WebViewImpl::paint(WebCanvas* canvas, const WebRect& rect)
{
    if (isAcceleratedCompositingActive()) {
#if USE(ACCELERATED_COMPOSITING)
        // If a canvas was passed in, we use it to grab a copy of the
        // freshly-rendered pixels.
        if (canvas) {
            // Clip rect to the confines of the rootLayerTexture.
            IntRect resizeRect(rect);
            resizeRect.intersect(IntRect(IntPoint(0, 0), m_layerTreeHost->viewportSize()));
            doPixelReadbackToCanvas(canvas, resizeRect);
        }
#endif
    } else {
        double paintStart = currentTime();
        WebFrameImpl* webframe = mainFrameImpl();
        if (webframe)
            webframe->paint(canvas, rect);
        double paintEnd = currentTime();
        double pixelsPerSec = (rect.width * rect.height) / (paintEnd - paintStart);
        PlatformSupport::histogramCustomCounts("Renderer4.SoftwarePaintDurationMS", (paintEnd - paintStart) * 1000, 0, 120, 30);
        PlatformSupport::histogramCustomCounts("Renderer4.SoftwarePaintMegapixPerSecond", pixelsPerSec / 1000000, 10, 210, 30);
    }
}

void WebViewImpl::themeChanged()
{
    if (!page())
        return;
    FrameView* view = page()->mainFrame()->view();

    WebRect damagedRect(0, 0, m_size.width, m_size.height);
    view->invalidateRect(damagedRect);
}

void WebViewImpl::composite(bool)
{
#if USE(ACCELERATED_COMPOSITING)
    if (settings()->useThreadedCompositor())
        m_layerTreeHost->setNeedsRedraw();
    else {
        ASSERT(isAcceleratedCompositingActive());
        if (!page())
            return;

        if (m_pageOverlay)
            m_pageOverlay->update();

        m_layerTreeHost->composite();
    }
#endif
}

void WebViewImpl::loseCompositorContext(int numTimes)
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_layerTreeHost)
        m_layerTreeHost->loseCompositorContext(numTimes);
#endif
}

void WebViewImpl::enterFullScreenForElement(WebCore::Element* element)
{
    // We are already transitioning to fullscreen for a different element.
    if (m_provisionalFullScreenElement) {
        m_provisionalFullScreenElement = element;
        return;
    }

    // We are already in fullscreen mode.
    if (m_fullScreenFrame) {
        m_provisionalFullScreenElement = element;
        willEnterFullScreen();
        didEnterFullScreen();
        return;
    }

    // We need to transition to fullscreen mode.
    if (m_client && m_client->enterFullScreen())
        m_provisionalFullScreenElement = element;
}

void WebViewImpl::exitFullScreenForElement(WebCore::Element* element)
{
    if (m_client)
        m_client->exitFullScreen();
}

const WebInputEvent* WebViewImpl::m_currentInputEvent = 0;

bool WebViewImpl::handleInputEvent(const WebInputEvent& inputEvent)
{
    UserGestureIndicator gestureIndicator(WebInputEvent::isUserGestureEventType(inputEvent.type) ? DefinitelyProcessingUserGesture : PossiblyProcessingUserGesture);

    // If we've started a drag and drop operation, ignore input events until
    // we're done.
    if (m_doingDragAndDrop)
        return true;

    if (m_ignoreInputEvents)
        return true;

    m_currentInputEvent = &inputEvent;

    if (m_mouseCaptureNode && WebInputEvent::isMouseEventType(inputEvent.type)) {
        // Save m_mouseCaptureNode since mouseCaptureLost() will clear it.
        RefPtr<Node> node = m_mouseCaptureNode;

        // Not all platforms call mouseCaptureLost() directly.
        if (inputEvent.type == WebInputEvent::MouseUp)
            mouseCaptureLost();

        AtomicString eventType;
        switch (inputEvent.type) {
        case WebInputEvent::MouseMove:
            eventType = eventNames().mousemoveEvent;
            break;
        case WebInputEvent::MouseLeave:
            eventType = eventNames().mouseoutEvent;
            break;
        case WebInputEvent::MouseDown:
            eventType = eventNames().mousedownEvent;
            break;
        case WebInputEvent::MouseUp:
            eventType = eventNames().mouseupEvent;
            break;
        default:
            ASSERT_NOT_REACHED();
        }

        node->dispatchMouseEvent(
              PlatformMouseEventBuilder(mainFrameImpl()->frameView(), *static_cast<const WebMouseEvent*>(&inputEvent)),
              eventType, static_cast<const WebMouseEvent*>(&inputEvent)->clickCount);
        m_currentInputEvent = 0;
        return true;
    }

    bool handled = true;

    // FIXME: WebKit seems to always return false on mouse events processing
    // methods. For now we'll assume it has processed them (as we are only
    // interested in whether keyboard events are processed).
    switch (inputEvent.type) {
    case WebInputEvent::MouseMove:
        mouseMove(*static_cast<const WebMouseEvent*>(&inputEvent));
        break;

    case WebInputEvent::MouseLeave:
        mouseLeave(*static_cast<const WebMouseEvent*>(&inputEvent));
        break;

    case WebInputEvent::MouseWheel:
        handled = mouseWheel(*static_cast<const WebMouseWheelEvent*>(&inputEvent));
        break;

    case WebInputEvent::MouseDown:
        mouseDown(*static_cast<const WebMouseEvent*>(&inputEvent));
        break;

    case WebInputEvent::MouseUp:
        mouseUp(*static_cast<const WebMouseEvent*>(&inputEvent));
        break;

    case WebInputEvent::RawKeyDown:
    case WebInputEvent::KeyDown:
    case WebInputEvent::KeyUp:
        handled = keyEvent(*static_cast<const WebKeyboardEvent*>(&inputEvent));
        break;

    case WebInputEvent::Char:
        handled = charEvent(*static_cast<const WebKeyboardEvent*>(&inputEvent));
        break;

#if ENABLE(GESTURE_EVENTS)
    case WebInputEvent::GestureScrollBegin:
    case WebInputEvent::GestureScrollEnd:
    case WebInputEvent::GestureScrollUpdate:
    case WebInputEvent::GestureFlingStart:
    case WebInputEvent::GestureFlingCancel:
    case WebInputEvent::GestureTap:
        handled = gestureEvent(*static_cast<const WebGestureEvent*>(&inputEvent));
        break;
#endif

#if ENABLE(TOUCH_EVENTS)
    case WebInputEvent::TouchStart:
    case WebInputEvent::TouchMove:
    case WebInputEvent::TouchEnd:
    case WebInputEvent::TouchCancel:
        handled = touchEvent(*static_cast<const WebTouchEvent*>(&inputEvent));
        break;
#endif

    default:
        handled = false;
    }

    m_currentInputEvent = 0;

    return handled;
}

void WebViewImpl::mouseCaptureLost()
{
    m_mouseCaptureNode = 0;
}

void WebViewImpl::setFocus(bool enable)
{
    m_page->focusController()->setFocused(enable);
    m_page->focusController()->setActive(enable);
    if (enable) {
        RefPtr<Frame> focusedFrame = m_page->focusController()->focusedFrame();
        if (focusedFrame) {
            Node* focusedNode = focusedFrame->document()->focusedNode();
            if (focusedNode && focusedNode->isElementNode()
                && focusedFrame->selection()->selection().isNone()) {
                // If the selection was cleared while the WebView was not
                // focused, then the focus element shows with a focus ring but
                // no caret and does respond to keyboard inputs.
                Element* element = static_cast<Element*>(focusedNode);
                if (element->isTextFormControl())
                    element->updateFocusAppearance(true);
                else if (focusedNode->isContentEditable()) {
                    // updateFocusAppearance() selects all the text of
                    // contentseditable DIVs. So we set the selection explicitly
                    // instead. Note that this has the side effect of moving the
                    // caret back to the beginning of the text.
                    Position position(focusedNode, 0,
                                      Position::PositionIsOffsetInAnchor);
                    focusedFrame->selection()->setSelection(
                        VisibleSelection(position, SEL_DEFAULT_AFFINITY));
                }
            }
        }
        m_imeAcceptEvents = true;
    } else {
        hideAutofillPopup();
        hideSelectPopup();

        // Clear focus on the currently focused frame if any.
        if (!m_page)
            return;

        Frame* frame = m_page->mainFrame();
        if (!frame)
            return;

        RefPtr<Frame> focusedFrame = m_page->focusController()->focusedFrame();
        if (focusedFrame) {
            // Finish an ongoing composition to delete the composition node.
            Editor* editor = focusedFrame->editor();
            if (editor && editor->hasComposition())
                editor->confirmComposition();
            m_imeAcceptEvents = false;
        }
    }
}

bool WebViewImpl::setComposition(
    const WebString& text,
    const WebVector<WebCompositionUnderline>& underlines,
    int selectionStart,
    int selectionEnd)
{
    Frame* focused = focusedWebCoreFrame();
    if (!focused || !m_imeAcceptEvents)
        return false;
    Editor* editor = focused->editor();
    if (!editor)
        return false;

    // The input focus has been moved to another WebWidget object.
    // We should use this |editor| object only to complete the ongoing
    // composition.
    if (!editor->canEdit() && !editor->hasComposition())
        return false;

    // We should verify the parent node of this IME composition node are
    // editable because JavaScript may delete a parent node of the composition
    // node. In this case, WebKit crashes while deleting texts from the parent
    // node, which doesn't exist any longer.
    PassRefPtr<Range> range = editor->compositionRange();
    if (range) {
        Node* node = range->startContainer();
        if (!node || !node->isContentEditable())
            return false;
    }

    // If we're not going to fire a keypress event, then the keydown event was
    // canceled.  In that case, cancel any existing composition.
    if (text.isEmpty() || m_suppressNextKeypressEvent) {
        // A browser process sent an IPC message which does not contain a valid
        // string, which means an ongoing composition has been canceled.
        // If the ongoing composition has been canceled, replace the ongoing
        // composition string with an empty string and complete it.
        String emptyString;
        Vector<CompositionUnderline> emptyUnderlines;
        editor->setComposition(emptyString, emptyUnderlines, 0, 0);
        return text.isEmpty();
    }

    // When the range of composition underlines overlap with the range between
    // selectionStart and selectionEnd, WebKit somehow won't paint the selection
    // at all (see InlineTextBox::paint() function in InlineTextBox.cpp).
    // But the selection range actually takes effect.
    editor->setComposition(String(text),
                           CompositionUnderlineVectorBuilder(underlines),
                           selectionStart, selectionEnd);

    return editor->hasComposition();
}

bool WebViewImpl::confirmComposition()
{
    return confirmComposition(WebString());
}

bool WebViewImpl::confirmComposition(const WebString& text)
{
    Frame* focused = focusedWebCoreFrame();
    if (!focused || !m_imeAcceptEvents)
        return false;
    Editor* editor = focused->editor();
    if (!editor || (!editor->hasComposition() && !text.length()))
        return false;

    // We should verify the parent node of this IME composition node are
    // editable because JavaScript may delete a parent node of the composition
    // node. In this case, WebKit crashes while deleting texts from the parent
    // node, which doesn't exist any longer.
    PassRefPtr<Range> range = editor->compositionRange();
    if (range) {
        Node* node = range->startContainer();
        if (!node || !node->isContentEditable())
            return false;
    }

    if (editor->hasComposition()) {
        if (text.length())
            editor->confirmComposition(String(text));
        else
            editor->confirmComposition();
    } else
        editor->insertText(String(text), 0);

    return true;
}

bool WebViewImpl::compositionRange(size_t* location, size_t* length)
{
    Frame* focused = focusedWebCoreFrame();
    if (!focused || !focused->selection() || !m_imeAcceptEvents)
        return false;
    Editor* editor = focused->editor();
    if (!editor || !editor->hasComposition())
        return false;

    RefPtr<Range> range = editor->compositionRange();
    if (!range)
        return false;

    if (TextIterator::getLocationAndLengthFromRange(focused->selection()->rootEditableElementOrDocumentElement(), range.get(), *location, *length))
        return true;
    return false;
}

WebTextInputType WebViewImpl::textInputType()
{
    Node* node = focusedWebCoreNode();
    if (!node)
        return WebTextInputTypeNone;

    if (node->nodeType() == Node::ELEMENT_NODE) {
        Element* element = static_cast<Element*>(node);
        if (element->hasLocalName(HTMLNames::inputTag)) {
            HTMLInputElement* input = static_cast<HTMLInputElement*>(element);

            if (input->readOnly() || input->disabled())
                return WebTextInputTypeNone;

            if (input->isPasswordField())
                return WebTextInputTypePassword;
            if (input->isSearchField())
                return WebTextInputTypeSearch;
            if (input->isEmailField())
                return WebTextInputTypeEmail;
            if (input->isNumberField())
                return WebTextInputTypeNumber;
            if (input->isTelephoneField())
                return WebTextInputTypeTelephone;
            if (input->isURLField())
                return WebTextInputTypeURL;
            if (input->isTextField())
                return WebTextInputTypeText;
            return WebTextInputTypeNone;
        }

        if (element->hasLocalName(HTMLNames::textareaTag)) {
            HTMLTextAreaElement* textarea = static_cast<HTMLTextAreaElement*>(element);

            if (textarea->readOnly() || textarea->disabled())
                return WebTextInputTypeNone;
            return WebTextInputTypeText;
        }
    }

    // For other situations.
    if (node->shouldUseInputMethod())
        return WebTextInputTypeText;

    return WebTextInputTypeNone;
}

bool WebViewImpl::selectionBounds(WebRect& start, WebRect& end) const
{
    const Frame* frame = focusedWebCoreFrame();
    if (!frame)
        return false;
    FrameSelection* selection = frame->selection();
    if (!selection)
        return false;

    if (selection->isCaret()) {
        start = end = frame->view()->contentsToWindow(selection->absoluteCaretBounds());
        return true;
    }

    RefPtr<Range> selectedRange = frame->selection()->toNormalizedRange();
    if (!selectedRange)
        return false;

    RefPtr<Range> range(Range::create(selectedRange->startContainer()->document(),
                                      selectedRange->startContainer(),
                                      selectedRange->startOffset(),
                                      selectedRange->startContainer(),
                                      selectedRange->startOffset()));
    start = frame->editor()->firstRectForRange(range.get());

    range = Range::create(selectedRange->endContainer()->document(),
                          selectedRange->endContainer(),
                          selectedRange->endOffset(),
                          selectedRange->endContainer(),
                          selectedRange->endOffset());
    end = frame->editor()->firstRectForRange(range.get());

    start = frame->view()->contentsToWindow(start);
    end = frame->view()->contentsToWindow(end);

    if (!frame->selection()->selection().isBaseFirst())
        std::swap(start, end);
    return true;
}

bool WebViewImpl::caretOrSelectionRange(size_t* location, size_t* length)
{
    const Frame* focused = focusedWebCoreFrame();
    if (!focused)
        return false;

    FrameSelection* selection = focused->selection();
    if (!selection)
        return false;

    RefPtr<Range> range = selection->selection().firstRange();
    if (!range)
        return false;

    if (TextIterator::getLocationAndLengthFromRange(selection->rootEditableElementOrDocumentElement(), range.get(), *location, *length))
        return true;
    return false;
}

void WebViewImpl::setTextDirection(WebTextDirection direction)
{
    // The Editor::setBaseWritingDirection() function checks if we can change
    // the text direction of the selected node and updates its DOM "dir"
    // attribute and its CSS "direction" property.
    // So, we just call the function as Safari does.
    const Frame* focused = focusedWebCoreFrame();
    if (!focused)
        return;

    Editor* editor = focused->editor();
    if (!editor || !editor->canEdit())
        return;

    switch (direction) {
    case WebTextDirectionDefault:
        editor->setBaseWritingDirection(NaturalWritingDirection);
        break;

    case WebTextDirectionLeftToRight:
        editor->setBaseWritingDirection(LeftToRightWritingDirection);
        break;

    case WebTextDirectionRightToLeft:
        editor->setBaseWritingDirection(RightToLeftWritingDirection);
        break;

    default:
        notImplemented();
        break;
    }
}

bool WebViewImpl::isAcceleratedCompositingActive() const
{
#if USE(ACCELERATED_COMPOSITING)
    return m_isAcceleratedCompositingActive;
#else
    return false;
#endif
}

// WebView --------------------------------------------------------------------

WebSettings* WebViewImpl::settings()
{
    if (!m_webSettings)
        m_webSettings = adoptPtr(new WebSettingsImpl(m_page->settings()));
    ASSERT(m_webSettings);
    return m_webSettings.get();
}

WebString WebViewImpl::pageEncoding() const
{
    if (!m_page)
        return WebString();

    // FIXME: Is this check needed?
    if (!m_page->mainFrame()->document()->loader())
        return WebString();

    return m_page->mainFrame()->document()->encoding();
}

void WebViewImpl::setPageEncoding(const WebString& encodingName)
{
    if (!m_page)
        return;

    // Only change override encoding, don't change default encoding.
    // Note that the new encoding must be 0 if it isn't supposed to be set.
    String newEncodingName;
    if (!encodingName.isEmpty())
        newEncodingName = encodingName;
    m_page->mainFrame()->loader()->reloadWithOverrideEncoding(newEncodingName);
}

bool WebViewImpl::dispatchBeforeUnloadEvent()
{
    // FIXME: This should really cause a recursive depth-first walk of all
    // frames in the tree, calling each frame's onbeforeunload.  At the moment,
    // we're consistent with Safari 3.1, not IE/FF.
    Frame* frame = m_page->mainFrame();
    if (!frame)
        return true;

    return frame->loader()->shouldClose();
}

void WebViewImpl::dispatchUnloadEvent()
{
    // Run unload handlers.
    m_page->mainFrame()->loader()->closeURL();
}

WebFrame* WebViewImpl::mainFrame()
{
    return mainFrameImpl();
}

WebFrame* WebViewImpl::findFrameByName(
    const WebString& name, WebFrame* relativeToFrame)
{
    if (!relativeToFrame)
        relativeToFrame = mainFrame();
    Frame* frame = static_cast<WebFrameImpl*>(relativeToFrame)->frame();
    frame = frame->tree()->find(name);
    return WebFrameImpl::fromFrame(frame);
}

WebFrame* WebViewImpl::focusedFrame()
{
    return WebFrameImpl::fromFrame(focusedWebCoreFrame());
}

void WebViewImpl::setFocusedFrame(WebFrame* frame)
{
    if (!frame) {
        // Clears the focused frame if any.
        Frame* frame = focusedWebCoreFrame();
        if (frame)
            frame->selection()->setFocused(false);
        return;
    }
    WebFrameImpl* frameImpl = static_cast<WebFrameImpl*>(frame);
    Frame* webcoreFrame = frameImpl->frame();
    webcoreFrame->page()->focusController()->setFocusedFrame(webcoreFrame);
}

void WebViewImpl::setInitialFocus(bool reverse)
{
    if (!m_page)
        return;

    // Since we don't have a keyboard event, we'll create one.
    WebKeyboardEvent keyboardEvent;
    keyboardEvent.type = WebInputEvent::RawKeyDown;
    if (reverse)
        keyboardEvent.modifiers = WebInputEvent::ShiftKey;

    // VK_TAB which is only defined on Windows.
    keyboardEvent.windowsKeyCode = 0x09;
    PlatformKeyboardEventBuilder platformEvent(keyboardEvent);
    RefPtr<KeyboardEvent> webkitEvent = KeyboardEvent::create(platformEvent, 0);

    Frame* frame = page()->focusController()->focusedOrMainFrame();
    if (Document* document = frame->document())
        document->setFocusedNode(0);
    page()->focusController()->setInitialFocus(
        reverse ? FocusDirectionBackward : FocusDirectionForward,
        webkitEvent.get());
}

void WebViewImpl::clearFocusedNode()
{
    RefPtr<Frame> frame = focusedWebCoreFrame();
    if (!frame)
        return;

    RefPtr<Document> document = frame->document();
    if (!document)
        return;

    RefPtr<Node> oldFocusedNode = document->focusedNode();

    // Clear the focused node.
    document->setFocusedNode(0);

    if (!oldFocusedNode)
        return;

    // If a text field has focus, we need to make sure the selection controller
    // knows to remove selection from it. Otherwise, the text field is still
    // processing keyboard events even though focus has been moved to the page and
    // keystrokes get eaten as a result.
    if (oldFocusedNode->isContentEditable()
        || (oldFocusedNode->isElementNode()
            && static_cast<Element*>(oldFocusedNode.get())->isTextFormControl())) {
        frame->selection()->clear();
    }
}

void WebViewImpl::scrollFocusedNodeIntoView()
{
    Node* focusedNode = focusedWebCoreNode();
    if (focusedNode && focusedNode->isElementNode()) {
        Element* elementNode = static_cast<Element*>(focusedNode);
        elementNode->scrollIntoViewIfNeeded(true);
    }
}

void WebViewImpl::scrollFocusedNodeIntoRect(const WebRect& rect)
{
    Frame* frame = page()->mainFrame();
    Node* focusedNode = focusedWebCoreNode();
    if (!frame || !frame->view() || !focusedNode || !focusedNode->isElementNode())
        return;
    Element* elementNode = static_cast<Element*>(focusedNode);
    frame->view()->scrollElementToRect(elementNode, IntRect(rect.x, rect.y, rect.width, rect.height));
}

double WebViewImpl::zoomLevel()
{
    return m_zoomLevel;
}

double WebViewImpl::setZoomLevel(bool textOnly, double zoomLevel)
{
    if (m_layerTreeHost)
        m_layerTreeHost->setZoomAnimatorTransform(TransformationMatrix());

    if (zoomLevel < m_minimumZoomLevel)
        m_zoomLevel = m_minimumZoomLevel;
    else if (zoomLevel > m_maximumZoomLevel)
        m_zoomLevel = m_maximumZoomLevel;
    else
        m_zoomLevel = zoomLevel;

    Frame* frame = mainFrameImpl()->frame();
    WebPluginContainerImpl* pluginContainer = WebFrameImpl::pluginContainerFromFrame(frame);
    if (pluginContainer)
        pluginContainer->plugin()->setZoomLevel(m_zoomLevel, textOnly);
    else {
        float zoomFactor = static_cast<float>(zoomLevelToZoomFactor(m_zoomLevel));
        if (textOnly)
            frame->setPageAndTextZoomFactors(1, zoomFactor);
        else
            frame->setPageAndTextZoomFactors(zoomFactor, 1);
    }
    return m_zoomLevel;
}

void WebViewImpl::zoomLimitsChanged(double minimumZoomLevel,
                                    double maximumZoomLevel)
{
    m_minimumZoomLevel = minimumZoomLevel;
    m_maximumZoomLevel = maximumZoomLevel;
    m_client->zoomLimitsChanged(m_minimumZoomLevel, m_maximumZoomLevel);
}

void WebViewImpl::fullFramePluginZoomLevelChanged(double zoomLevel)
{
    if (zoomLevel == m_zoomLevel)
        return;

    m_zoomLevel = max(min(zoomLevel, m_maximumZoomLevel), m_minimumZoomLevel);
    m_client->zoomLevelChanged();
}

double WebView::zoomLevelToZoomFactor(double zoomLevel)
{
    return pow(textSizeMultiplierRatio, zoomLevel);
}

double WebView::zoomFactorToZoomLevel(double factor)
{
    // Since factor = 1.2^level, level = log(factor) / log(1.2)
    return log(factor) / log(textSizeMultiplierRatio);
}

float WebViewImpl::pageScaleFactor() const
{
    if (!page())
        return 1;

    return page()->pageScaleFactor();
}

float WebViewImpl::computePageScaleFactorWithinLimits(float scaleFactor)
{
    return min(max(scaleFactor, m_minimumPageScaleFactor), m_maximumPageScaleFactor);
}

WebPoint WebViewImpl::clampOffsetAtScale(const WebPoint& offset, float scale)
{
    // This is the scaled content size. We need to convert it to the new scale factor.
    WebSize contentSize = mainFrame()->contentsSize();
    float deltaScale = scale / pageScaleFactor();
    int docWidthAtNewScale = contentSize.width * deltaScale;
    int docHeightAtNewScale = contentSize.height * deltaScale;
    int viewWidth = m_size.width;
    int viewHeight = m_size.height;

    // Enforce the maximum and minimum scroll positions at the new scale.
    IntPoint clampedOffset = offset;
    clampedOffset.clampNegativeToZero();
    clampedOffset = clampedOffset.shrunkTo(IntPoint(docWidthAtNewScale - viewWidth, docHeightAtNewScale - viewHeight));
    return clampedOffset;
}

void WebViewImpl::setPageScaleFactorPreservingScrollOffset(float scaleFactor)
{
    // Pick a scale factor that is within the expected limits
    scaleFactor = computePageScaleFactorWithinLimits(scaleFactor);
    if (scaleFactor == pageScaleFactor())
        return;

    IntPoint scrollOffsetAtNewScale(mainFrame()->scrollOffset().width, mainFrame()->scrollOffset().height);
    float deltaScale = scaleFactor / pageScaleFactor();
    scrollOffsetAtNewScale.scale(deltaScale, deltaScale);

    WebPoint clampedOffsetAtNewScale = clampOffsetAtScale(scrollOffsetAtNewScale, scaleFactor);
    setPageScaleFactor(scaleFactor, clampedOffsetAtNewScale);
}

void WebViewImpl::setPageScaleFactor(float scaleFactor, const WebPoint& origin)
{
    if (!page())
        return;

    if (!scaleFactor)
        scaleFactor = 1;

    scaleFactor = computePageScaleFactorWithinLimits(scaleFactor);
    WebPoint clampedOrigin = clampOffsetAtScale(origin, scaleFactor);
    page()->setPageScaleFactor(scaleFactor, clampedOrigin);
}

float WebViewImpl::deviceScaleFactor() const
{
    if (!page())
        return 1;

    return page()->deviceScaleFactor();
}

void WebViewImpl::setDeviceScaleFactor(float scaleFactor)
{
    if (!page())
        return;

    page()->setDeviceScaleFactor(scaleFactor);
}

bool WebViewImpl::isFixedLayoutModeEnabled() const
{
    if (!page())
        return false;

    Frame* frame = page()->mainFrame();
    if (!frame || !frame->view())
        return false;

    return frame->view()->useFixedLayout();
}

void WebViewImpl::enableFixedLayoutMode(bool enable)
{
    if (!page())
        return;

    Frame* frame = page()->mainFrame();
    if (!frame || !frame->view())
        return;

    frame->view()->setUseFixedLayout(enable);

#if USE(ACCELERATED_COMPOSITING)
    // Also notify the base layer, which RenderLayerCompositor does not see.
    if (m_nonCompositedContentHost) {
        m_nonCompositedContentHost->topLevelRootLayer()->deviceOrPageScaleFactorChanged();
        updateLayerTreeViewport();
    }
#endif
}

void WebViewImpl::setPageScaleFactorLimits(float minPageScale, float maxPageScale)
{
    m_minimumPageScaleFactor = min(max(minPageScale, minPageScaleFactor), maxPageScaleFactor) * deviceScaleFactor();
    m_maximumPageScaleFactor = max(min(maxPageScale, maxPageScaleFactor), minPageScaleFactor) * deviceScaleFactor();

    if (m_size.width && mainFrame() && mainFrame()->contentsSize().width) {
        // Limit page scaling down to the document width.
        int viewWidth = m_size.width;
        int unscaledContentWidth = mainFrame()->contentsSize().width / pageScaleFactor();
        m_minimumPageScaleFactor = max(m_minimumPageScaleFactor, static_cast<float>(viewWidth) / unscaledContentWidth);
    }
    ASSERT(minPageScale <= maxPageScale);
#if USE(ACCELERATED_COMPOSITING)
    if (m_layerTreeHost)
        m_layerTreeHost->setPageScaleFactorLimits(m_minimumPageScaleFactor, m_maximumPageScaleFactor);
#endif

    float clampedScale = computePageScaleFactorWithinLimits(pageScaleFactor());
    if (clampedScale != pageScaleFactor())
        setPageScaleFactorPreservingScrollOffset(clampedScale);
}

float WebViewImpl::minimumPageScaleFactor() const
{
    return m_minimumPageScaleFactor;
}

float WebViewImpl::maximumPageScaleFactor() const
{
    return m_maximumPageScaleFactor;
}

WebSize WebViewImpl::fixedLayoutSize() const
{
    if (!page())
        return WebSize();

    Frame* frame = page()->mainFrame();
    if (!frame || !frame->view())
        return WebSize();

    return frame->view()->fixedLayoutSize();
}

void WebViewImpl::setFixedLayoutSize(const WebSize& layoutSize)
{
    if (!page())
        return;

    Frame* frame = page()->mainFrame();
    if (!frame || !frame->view())
        return;

    frame->view()->setFixedLayoutSize(layoutSize);
}

void WebViewImpl::performMediaPlayerAction(const WebMediaPlayerAction& action,
                                           const WebPoint& location)
{
    HitTestResult result = hitTestResultForWindowPos(location);
    RefPtr<Node> node = result.innerNonSharedNode();
    if (!node->hasTagName(HTMLNames::videoTag) && !node->hasTagName(HTMLNames::audioTag))
      return;

    RefPtr<HTMLMediaElement> mediaElement =
        static_pointer_cast<HTMLMediaElement>(node);
    switch (action.type) {
    case WebMediaPlayerAction::Play:
        if (action.enable)
            mediaElement->play();
        else
            mediaElement->pause();
        break;
    case WebMediaPlayerAction::Mute:
        mediaElement->setMuted(action.enable);
        break;
    case WebMediaPlayerAction::Loop:
        mediaElement->setLoop(action.enable);
        break;
    case WebMediaPlayerAction::Controls:
        mediaElement->setControls(action.enable);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

void WebViewImpl::copyImageAt(const WebPoint& point)
{
    if (!m_page)
        return;

    HitTestResult result = hitTestResultForWindowPos(point);

    if (result.absoluteImageURL().isEmpty()) {
        // There isn't actually an image at these coordinates.  Might be because
        // the window scrolled while the context menu was open or because the page
        // changed itself between when we thought there was an image here and when
        // we actually tried to retreive the image.
        //
        // FIXME: implement a cache of the most recent HitTestResult to avoid having
        //        to do two hit tests.
        return;
    }

    m_page->mainFrame()->editor()->copyImage(result);
}

void WebViewImpl::dragSourceEndedAt(
    const WebPoint& clientPoint,
    const WebPoint& screenPoint,
    WebDragOperation operation)
{
    PlatformMouseEvent pme(clientPoint,
                           screenPoint,
                           LeftButton, MouseEventMoved, 0, false, false, false,
                           false, 0);
    m_page->mainFrame()->eventHandler()->dragSourceEndedAt(pme,
        static_cast<DragOperation>(operation));
    m_dragScrollTimer->stop();
}

void WebViewImpl::dragSourceMovedTo(
    const WebPoint& clientPoint,
    const WebPoint& screenPoint,
    WebDragOperation operation)
{
    m_dragScrollTimer->triggerScroll(mainFrameImpl()->frameView(), clientPoint);
}

void WebViewImpl::dragSourceSystemDragEnded()
{
    // It's possible for us to get this callback while not doing a drag if
    // it's from a previous page that got unloaded.
    if (m_doingDragAndDrop) {
        m_page->dragController()->dragEnded();
        m_doingDragAndDrop = false;
    }
}

WebDragOperation WebViewImpl::dragTargetDragEnter(
    const WebDragData& webDragData,
    const WebPoint& clientPoint,
    const WebPoint& screenPoint,
    WebDragOperationsMask operationsAllowed)
{
    ASSERT(!m_currentDragData);

    m_currentDragData = webDragData;
    m_operationsAllowed = operationsAllowed;

    return dragTargetDragEnterOrOver(clientPoint, screenPoint, DragEnter);
}

WebDragOperation WebViewImpl::dragTargetDragOver(
    const WebPoint& clientPoint,
    const WebPoint& screenPoint,
    WebDragOperationsMask operationsAllowed)
{
    m_operationsAllowed = operationsAllowed;

    return dragTargetDragEnterOrOver(clientPoint, screenPoint, DragOver);
}

void WebViewImpl::dragTargetDragLeave()
{
    ASSERT(m_currentDragData);

    DragData dragData(
        m_currentDragData.get(),
        IntPoint(),
        IntPoint(),
        static_cast<DragOperation>(m_operationsAllowed));

    m_page->dragController()->dragExited(&dragData);

    // FIXME: why is the drag scroll timer not stopped here?

    m_dragOperation = WebDragOperationNone;
    m_currentDragData = 0;
}

void WebViewImpl::dragTargetDrop(const WebPoint& clientPoint,
                                 const WebPoint& screenPoint)
{
    ASSERT(m_currentDragData);

    // If this webview transitions from the "drop accepting" state to the "not
    // accepting" state, then our IPC message reply indicating that may be in-
    // flight, or else delayed by javascript processing in this webview.  If a
    // drop happens before our IPC reply has reached the browser process, then
    // the browser forwards the drop to this webview.  So only allow a drop to
    // proceed if our webview m_dragOperation state is not DragOperationNone.

    if (m_dragOperation == WebDragOperationNone) { // IPC RACE CONDITION: do not allow this drop.
        dragTargetDragLeave();
        return;
    }

    DragData dragData(
        m_currentDragData.get(),
        clientPoint,
        screenPoint,
        static_cast<DragOperation>(m_operationsAllowed));

    m_page->dragController()->performDrag(&dragData);

    m_dragOperation = WebDragOperationNone;
    m_currentDragData = 0;

    m_dragScrollTimer->stop();
}

WebDragOperation WebViewImpl::dragTargetDragEnterOrOver(const WebPoint& clientPoint, const WebPoint& screenPoint, DragAction dragAction)
{
    ASSERT(m_currentDragData);

    DragData dragData(
        m_currentDragData.get(),
        clientPoint,
        screenPoint,
        static_cast<DragOperation>(m_operationsAllowed));

    DragSession dragSession;
    if (dragAction == DragEnter)
        dragSession = m_page->dragController()->dragEntered(&dragData);
    else
        dragSession = m_page->dragController()->dragUpdated(&dragData);

    DragOperation dropEffect = dragSession.operation;

    // Mask the drop effect operation against the drag source's allowed operations.
    if (!(dropEffect & dragData.draggingSourceOperationMask()))
        dropEffect = DragOperationNone;

     m_dragOperation = static_cast<WebDragOperation>(dropEffect);

    if (dragAction == DragOver)
        m_dragScrollTimer->triggerScroll(mainFrameImpl()->frameView(), clientPoint);
    else
        m_dragScrollTimer->stop();

    return m_dragOperation;
}

unsigned long WebViewImpl::createUniqueIdentifierForRequest()
{
    if (m_page)
        return m_page->progress()->createUniqueIdentifier();
    return 0;
}

void WebViewImpl::inspectElementAt(const WebPoint& point)
{
    if (!m_page)
        return;

    if (point.x == -1 || point.y == -1)
        m_page->inspectorController()->inspect(0);
    else {
        HitTestResult result = hitTestResultForWindowPos(point);

        if (!result.innerNonSharedNode())
            return;

        m_page->inspectorController()->inspect(result.innerNonSharedNode());
    }
}

WebString WebViewImpl::inspectorSettings() const
{
    return m_inspectorSettings;
}

void WebViewImpl::setInspectorSettings(const WebString& settings)
{
    m_inspectorSettings = settings;
}

bool WebViewImpl::inspectorSetting(const WebString& key, WebString* value) const
{
    if (!m_inspectorSettingsMap->contains(key))
        return false;
    *value = m_inspectorSettingsMap->get(key);
    return true;
}

void WebViewImpl::setInspectorSetting(const WebString& key,
                                      const WebString& value)
{
    m_inspectorSettingsMap->set(key, value);
    client()->didUpdateInspectorSetting(key, value);
}

WebDevToolsAgent* WebViewImpl::devToolsAgent()
{
    return m_devToolsAgent.get();
}

WebAccessibilityObject WebViewImpl::accessibilityObject()
{
    if (!mainFrameImpl())
        return WebAccessibilityObject();

    Document* document = mainFrameImpl()->frame()->document();
    return WebAccessibilityObject(
        document->axObjectCache()->getOrCreate(document->renderer()));
}

void WebViewImpl::applyAutofillSuggestions(
    const WebNode& node,
    const WebVector<WebString>& names,
    const WebVector<WebString>& labels,
    const WebVector<WebString>& icons,
    const WebVector<int>& uniqueIDs,
    int separatorIndex)
{
    ASSERT(names.size() == labels.size());
    ASSERT(names.size() == uniqueIDs.size());
    ASSERT(separatorIndex < static_cast<int>(names.size()));

    if (names.isEmpty()) {
        hideAutofillPopup();
        return;
    }

    RefPtr<Node> focusedNode = focusedWebCoreNode();
    // If the node for which we queried the Autofill suggestions is not the
    // focused node, then we have nothing to do.  FIXME: also check the
    // caret is at the end and that the text has not changed.
    if (!focusedNode || focusedNode != PassRefPtr<Node>(node)) {
        hideAutofillPopup();
        return;
    }

    HTMLInputElement* inputElem = focusedNode->toInputElement();
    ASSERT(inputElem);

    // The first time the Autofill popup is shown we'll create the client and
    // the popup.
    if (!m_autofillPopupClient)
        m_autofillPopupClient = adoptPtr(new AutofillPopupMenuClient);

    m_autofillPopupClient->initialize(
        inputElem, names, labels, icons, uniqueIDs, separatorIndex);

    if (!m_autofillPopup) {
        m_autofillPopup = PopupContainer::create(m_autofillPopupClient.get(),
                                                 PopupContainer::Suggestion,
                                                 autofillPopupSettings);
    }

    if (m_autofillPopupShowing) {
        refreshAutofillPopup();
    } else {
        m_autofillPopup->showInRect(focusedNode->getRect(), focusedNode->ownerDocument()->view(), 0);
        m_autofillPopupShowing = true;
    }
}

void WebViewImpl::hidePopups()
{
    hideSelectPopup();
    hideAutofillPopup();
}

void WebViewImpl::performCustomContextMenuAction(unsigned action)
{
    if (!m_page)
        return;
    ContextMenu* menu = m_page->contextMenuController()->contextMenu();
    if (!menu)
        return;
    ContextMenuItem* item = menu->itemWithAction(static_cast<ContextMenuAction>(ContextMenuItemBaseCustomTag + action));
    if (item)
        m_page->contextMenuController()->contextMenuItemSelected(item);
    m_page->contextMenuController()->clearContextMenu();
}

// WebView --------------------------------------------------------------------

void WebViewImpl::setIsTransparent(bool isTransparent)
{
    // Set any existing frames to be transparent.
    Frame* frame = m_page->mainFrame();
    while (frame) {
        frame->view()->setTransparent(isTransparent);
        frame = frame->tree()->traverseNext();
    }

    // Future frames check this to know whether to be transparent.
    m_isTransparent = isTransparent;
}

bool WebViewImpl::isTransparent() const
{
    return m_isTransparent;
}

void WebViewImpl::setIsActive(bool active)
{
    if (page() && page()->focusController())
        page()->focusController()->setActive(active);
}

bool WebViewImpl::isActive() const
{
    return (page() && page()->focusController()) ? page()->focusController()->isActive() : false;
}

void WebViewImpl::setDomainRelaxationForbidden(bool forbidden, const WebString& scheme)
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, String(scheme));
}

void WebViewImpl::setScrollbarColors(unsigned inactiveColor,
                                     unsigned activeColor,
                                     unsigned trackColor) {
#if OS(UNIX) && !OS(DARWIN)
    PlatformThemeChromiumLinux::setScrollbarColors(inactiveColor, activeColor, trackColor);
#endif
}

void WebViewImpl::setSelectionColors(unsigned activeBackgroundColor,
                                     unsigned activeForegroundColor,
                                     unsigned inactiveBackgroundColor,
                                     unsigned inactiveForegroundColor) {
#if OS(UNIX) && !OS(DARWIN)
    RenderThemeChromiumLinux::setSelectionColors(activeBackgroundColor,
                                                 activeForegroundColor,
                                                 inactiveBackgroundColor,
                                                 inactiveForegroundColor);
    theme()->platformColorsDidChange();
#endif
}

void WebView::addUserScript(const WebString& sourceCode,
                            const WebVector<WebString>& patternsIn,
                            WebView::UserScriptInjectAt injectAt,
                            WebView::UserContentInjectIn injectIn)
{
    OwnPtr<Vector<String> > patterns = adoptPtr(new Vector<String>);
    for (size_t i = 0; i < patternsIn.size(); ++i)
        patterns->append(patternsIn[i]);

    PageGroup* pageGroup = PageGroup::pageGroup(pageGroupName);
    RefPtr<DOMWrapperWorld> world(DOMWrapperWorld::create());
    pageGroup->addUserScriptToWorld(world.get(), sourceCode, WebURL(), patterns.release(), nullptr,
                                    static_cast<UserScriptInjectionTime>(injectAt),
                                    static_cast<UserContentInjectedFrames>(injectIn));
}

void WebView::addUserStyleSheet(const WebString& sourceCode,
                                const WebVector<WebString>& patternsIn,
                                WebView::UserContentInjectIn injectIn,
                                WebView::UserStyleInjectionTime injectionTime)
{
    OwnPtr<Vector<String> > patterns = adoptPtr(new Vector<String>);
    for (size_t i = 0; i < patternsIn.size(); ++i)
        patterns->append(patternsIn[i]);

    PageGroup* pageGroup = PageGroup::pageGroup(pageGroupName);
    RefPtr<DOMWrapperWorld> world(DOMWrapperWorld::create());

    // FIXME: Current callers always want the level to be "author". It probably makes sense to let
    // callers specify this though, since in other cases the caller will probably want "user" level.
    //
    // FIXME: It would be nice to populate the URL correctly, instead of passing an empty URL.
    pageGroup->addUserStyleSheetToWorld(world.get(), sourceCode, WebURL(), patterns.release(), nullptr,
                                        static_cast<UserContentInjectedFrames>(injectIn),
                                        UserStyleAuthorLevel,
                                        static_cast<WebCore::UserStyleInjectionTime>(injectionTime));
}

void WebView::removeAllUserContent()
{
    PageGroup* pageGroup = PageGroup::pageGroup(pageGroupName);
    pageGroup->removeAllUserContent();
}

void WebViewImpl::didCommitLoad(bool* isNewNavigation)
{
    if (isNewNavigation)
        *isNewNavigation = m_observedNewNavigation;

#ifndef NDEBUG
    ASSERT(!m_observedNewNavigation
        || m_page->mainFrame()->loader()->documentLoader() == m_newNavigationLoader);
    m_newNavigationLoader = 0;
#endif
    m_observedNewNavigation = false;
}

bool WebViewImpl::useExternalPopupMenus()
{
    return shouldUseExternalPopupMenus;
}

bool WebViewImpl::navigationPolicyFromMouseEvent(unsigned short button,
                                                 bool ctrl, bool shift,
                                                 bool alt, bool meta,
                                                 WebNavigationPolicy* policy)
{
#if OS(DARWIN)
    const bool newTabModifier = (button == 1) || meta;
#else
    const bool newTabModifier = (button == 1) || ctrl;
#endif
    if (!newTabModifier && !shift && !alt)
      return false;

    ASSERT(policy);
    if (newTabModifier) {
        if (shift)
          *policy = WebNavigationPolicyNewForegroundTab;
        else
          *policy = WebNavigationPolicyNewBackgroundTab;
    } else {
        if (shift)
          *policy = WebNavigationPolicyNewWindow;
        else
          *policy = WebNavigationPolicyDownload;
    }
    return true;
}

void WebViewImpl::startDragging(const WebDragData& dragData,
                                WebDragOperationsMask mask,
                                const WebImage& dragImage,
                                const WebPoint& dragImageOffset)
{
    if (!m_client)
        return;
    ASSERT(!m_doingDragAndDrop);
    m_doingDragAndDrop = true;
    m_client->startDragging(dragData, mask, dragImage, dragImageOffset);
}

void WebViewImpl::observeNewNavigation()
{
    m_observedNewNavigation = true;
#ifndef NDEBUG
    m_newNavigationLoader = m_page->mainFrame()->loader()->documentLoader();
#endif
}

void WebViewImpl::setIgnoreInputEvents(bool newValue)
{
    ASSERT(m_ignoreInputEvents != newValue);
    m_ignoreInputEvents = newValue;
}

void WebViewImpl::setPageOverlayClient(PageOverlay::PageOverlayClient* pageOverlayClient)
{
    if (pageOverlayClient) {
        if (!m_pageOverlay)
            m_pageOverlay = PageOverlay::create(this, pageOverlayClient);
        else
            m_pageOverlay->setClient(pageOverlayClient);
        m_pageOverlay->update();
        setRootLayerNeedsDisplay();
    } else {
        if (m_pageOverlay) {
            m_pageOverlay->clear();
            m_pageOverlay = nullptr;
            setRootLayerNeedsDisplay();
        }
    }
}

void WebViewImpl::setOverlayLayer(WebCore::GraphicsLayer* layer)
{
    if (m_rootGraphicsLayer) {
        if (layer->parent() != m_rootGraphicsLayer)
            m_rootGraphicsLayer->addChild(layer);
    }
}

#if ENABLE(NOTIFICATIONS)
NotificationPresenterImpl* WebViewImpl::notificationPresenterImpl()
{
    if (!m_notificationPresenter.isInitialized() && m_client)
        m_notificationPresenter.initialize(m_client->notificationPresenter());
    return &m_notificationPresenter;
}
#endif

void WebViewImpl::refreshAutofillPopup()
{
    ASSERT(m_autofillPopupShowing);

    // Hide the popup if it has become empty.
    if (!m_autofillPopupClient->listSize()) {
        hideAutofillPopup();
        return;
    }

    IntRect oldBounds = m_autofillPopup->frameRect();
    m_autofillPopup->refresh(focusedWebCoreNode()->getRect());
    IntRect newBounds = m_autofillPopup->frameRect();
    // Let's resize the backing window if necessary.
    if (oldBounds != newBounds) {
        WebPopupMenuImpl* popupMenu =
            static_cast<WebPopupMenuImpl*>(m_autofillPopup->client());
        if (popupMenu)
            popupMenu->client()->setWindowRect(m_chromeClientImpl.rootViewToScreen(newBounds));
    }
}

Node* WebViewImpl::focusedWebCoreNode()
{
    Frame* frame = m_page->focusController()->focusedFrame();
    if (!frame)
        return 0;

    Document* document = frame->document();
    if (!document)
        return 0;

    return document->focusedNode();
}

HitTestResult WebViewImpl::hitTestResultForWindowPos(const IntPoint& pos)
{
    IntPoint docPoint(m_page->mainFrame()->view()->windowToContents(pos));
    return m_page->mainFrame()->eventHandler()->hitTestResultAtPoint(docPoint, false);
}

void WebViewImpl::setTabsToLinks(bool enable)
{
    m_tabsToLinks = enable;
}

bool WebViewImpl::tabsToLinks() const
{
    return m_tabsToLinks;
}

#if USE(ACCELERATED_COMPOSITING)
bool WebViewImpl::allowsAcceleratedCompositing()
{
    return !m_compositorCreationFailed;
}

bool WebViewImpl::pageHasRTLStyle() const
{
    if (!page())
        return false;
    Document* document = page()->mainFrame()->document();
    if (!document)
        return false;
    RenderView* renderView = document->renderView();
    if (!renderView)
        return false;
    RenderStyle* style = renderView->style();
    if (!style)
        return false;
    return (style->direction() == RTL);
}

void WebViewImpl::setRootGraphicsLayer(GraphicsLayer* layer)
{
    m_rootGraphicsLayer = layer;

    setIsAcceleratedCompositingActive(layer);
    if (m_nonCompositedContentHost) {
        GraphicsLayer* scrollLayer = 0;
        if (layer) {
            Document* document = page()->mainFrame()->document();
            RenderView* renderView = document->renderView();
            RenderLayerCompositor* compositor = renderView->compositor();
            scrollLayer = compositor->scrollLayer();
        }
        m_nonCompositedContentHost->setScrollLayer(scrollLayer);
    }
    if (m_layerTreeHost)
        m_layerTreeHost->setRootLayer(layer ? layer->platformLayer() : 0);

    IntRect damagedRect(0, 0, m_size.width, m_size.height);
    if (!m_isAcceleratedCompositingActive)
        m_client->didInvalidateRect(damagedRect);
}

void WebViewImpl::setRootLayerNeedsDisplay()
{
    if (m_layerTreeHost) {
        TransformationMatrix zoomMatrix;
        FloatPoint trans = m_page->mainFrame()->view()->scrollAnimator()->zoomTranslation();
        zoomMatrix.translate(trans.x(), trans.y());
        zoomMatrix.scale(m_page->mainFrame()->view()->scrollAnimator()->zoomScale());
        m_layerTreeHost->setZoomAnimatorTransform(zoomMatrix);
    }
    if (m_layerTreeHost)
        m_layerTreeHost->setNeedsCommit();
}

void WebViewImpl::scrollRootLayerRect(const IntSize& scrollDelta, const IntRect& clipRect)
{
    updateLayerTreeViewport();
    setRootLayerNeedsDisplay();
}

void WebViewImpl::invalidateRootLayerRect(const IntRect& rect)
{
    ASSERT(m_layerTreeHost);

    if (!page())
        return;

    FrameView* view = page()->mainFrame()->view();
    IntRect dirtyRect = view->windowToContents(rect);
    updateLayerTreeViewport();
    m_nonCompositedContentHost->invalidateRect(dirtyRect);
    setRootLayerNeedsDisplay();
}

WebCore::NonCompositedContentHost* WebViewImpl::nonCompositedContentHost()
{
    return m_nonCompositedContentHost.get();
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void WebViewImpl::scheduleAnimation()
{
    if (isAcceleratedCompositingActive()) {
        if (settings()->useThreadedCompositor()) {
            ASSERT(m_layerTreeHost);
            m_layerTreeHost->setNeedsAnimate();
        } else
            m_client->scheduleAnimation();
    } else
            m_client->scheduleAnimation();
}
#endif

class WebViewImplContentPainter : public LayerPainterChromium {
    WTF_MAKE_NONCOPYABLE(WebViewImplContentPainter);
public:
    static PassOwnPtr<WebViewImplContentPainter*> create(WebViewImpl* webViewImpl)
    {
        return adoptPtr(new WebViewImplContentPainter(webViewImpl));
    }

    virtual void paint(GraphicsContext& context, const IntRect& contentRect)
    {
        double paintStart = currentTime();
        Page* page = m_webViewImpl->page();
        if (!page)
            return;
        FrameView* view = page->mainFrame()->view();
        view->paintContents(&context, contentRect);
        double paintEnd = currentTime();
        double pixelsPerSec = (contentRect.width() * contentRect.height()) / (paintEnd - paintStart);
        PlatformSupport::histogramCustomCounts("Renderer4.AccelRootPaintDurationMS", (paintEnd - paintStart) * 1000, 0, 120, 30);
        PlatformSupport::histogramCustomCounts("Renderer4.AccelRootPaintMegapixPerSecond", pixelsPerSec / 1000000, 10, 210, 30);

        m_webViewImpl->nonCompositedContentHost()->setBackgroundColor(view->documentBackgroundColor());

    }

private:
    explicit WebViewImplContentPainter(WebViewImpl* webViewImpl)
        : m_webViewImpl(webViewImpl)
    {
    }

    WebViewImpl* m_webViewImpl;
};

void WebViewImpl::setIsAcceleratedCompositingActive(bool active)
{
    PlatformSupport::histogramEnumeration("GPU.setIsAcceleratedCompositingActive", active * 2 + m_isAcceleratedCompositingActive, 4);

    if (m_isAcceleratedCompositingActive == active)
        return;

    if (!active) {
        m_isAcceleratedCompositingActive = false;
        // We need to finish all GL rendering before sending didDeactivateCompositor() to prevent
        // flickering when compositing turns off.
        if (m_layerTreeHost)
            m_layerTreeHost->finishAllRendering();
        m_client->didDeactivateCompositor();
    } else if (m_layerTreeHost) {
        m_isAcceleratedCompositingActive = true;
        updateLayerTreeViewport();

        m_client->didActivateCompositor(m_layerTreeHost->compositorIdentifier());
    } else {
        TRACE_EVENT("WebViewImpl::setIsAcceleratedCompositingActive(true)", this, 0);

        static const double defaultRefreshRate = 60.0;

        WebCore::CCSettings ccSettings;
        ccSettings.acceleratePainting = page()->settings()->acceleratedDrawingEnabled();
        ccSettings.compositeOffscreen = settings()->compositeToTextureEnabled();
        ccSettings.enableCompositorThread = settings()->useThreadedCompositor();
        ccSettings.showFPSCounter = settings()->showFPSCounter();
        ccSettings.showPlatformLayerTree = settings()->showPlatformLayerTree();
        ccSettings.refreshRate = screenRefreshRate(page()->mainFrame()->view());
        ASSERT(ccSettings.refreshRate >= 0);
        if (!ccSettings.refreshRate)
            ccSettings.refreshRate = defaultRefreshRate;

        m_nonCompositedContentHost = NonCompositedContentHost::create(WebViewImplContentPainter::create(this));
        m_layerTreeHost = CCLayerTreeHost::create(this, ccSettings);
        if (m_layerTreeHost) {
            m_layerTreeHost->setHaveWheelEventHandlers(m_haveWheelEventHandlers);
            m_layerTreeHost->setPageScaleFactorLimits(m_minimumPageScaleFactor, m_maximumPageScaleFactor);
            updateLayerTreeViewport();
            m_client->didActivateCompositor(m_layerTreeHost->compositorIdentifier());
            m_isAcceleratedCompositingActive = true;
            m_compositorCreationFailed = false;
            if (m_pageOverlay)
                m_pageOverlay->update();
        } else {
            m_layerTreeHost.clear();
            m_nonCompositedContentHost.clear();
            m_isAcceleratedCompositingActive = false;
            m_client->didDeactivateCompositor();
            m_compositorCreationFailed = true;
        }
    }
    if (page())
        page()->mainFrame()->view()->setClipsRepaints(!m_isAcceleratedCompositingActive);
}

#endif

PassRefPtr<GraphicsContext3D> WebViewImpl::createLayerTreeHostContext3D()
{
    RefPtr<GraphicsContext3D> context = m_temporaryOnscreenGraphicsContext3D.release();
    if (!context) {
        if (settings()->useThreadedCompositor())
            context = GraphicsContext3DPrivate::createGraphicsContextForAnotherThread(getCompositorContextAttributes(), m_page->chrome(), GraphicsContext3D::RenderDirectlyToHostWindow);
        else
            context = GraphicsContext3D::create(getCompositorContextAttributes(), m_page->chrome(), GraphicsContext3D::RenderDirectlyToHostWindow);
    }
    return context;
}

void WebViewImpl::animateAndLayout(double frameBeginTime)
{
    animate(frameBeginTime);
    layout();
}

void WebViewImpl::applyScrollAndScale(const IntSize& scrollDelta, float pageScaleDelta)
{
    if (!mainFrameImpl() || !mainFrameImpl()->frameView())
        return;

    if (pageScaleDelta == 1)
        mainFrameImpl()->frameView()->scrollBy(scrollDelta);
    else {
        // The page scale changed, so apply a scale and scroll in a single
        // operation. The old scroll offset (and passed-in delta) are
        // in the old coordinate space, so we first need to multiply them
        // by the page scale delta.
        WebSize scrollOffset = mainFrame()->scrollOffset();
        scrollOffset.width += scrollDelta.width();
        scrollOffset.height += scrollDelta.height();
        WebPoint scaledScrollOffset(scrollOffset.width * pageScaleDelta,
                                    scrollOffset.height * pageScaleDelta);
        setPageScaleFactor(pageScaleFactor() * pageScaleDelta, scaledScrollOffset);
    }
}

void WebViewImpl::didCommitAndDrawFrame()
{
    if (m_client)
        m_client->didCommitAndDrawCompositorFrame();
}

void WebViewImpl::didCompleteSwapBuffers()
{
    if (m_client)
        m_client->didCompleteSwapBuffers();
}

void WebViewImpl::didRecreateGraphicsContext(bool success)
{

    // Switch back to software rendering mode, if necessary
    if (!success) {
        ASSERT(m_isAcceleratedCompositingActive);
        setIsAcceleratedCompositingActive(false);
        m_compositorCreationFailed = true;
        m_client->didInvalidateRect(IntRect(0, 0, m_size.width, m_size.height));

        // Force a style recalc to remove all the composited layers.
        m_page->mainFrame()->document()->scheduleForcedStyleRecalc();
        return;
    }

    if (m_pageOverlay)
        m_pageOverlay->update();
}

void WebViewImpl::scheduleComposite()
{
    ASSERT(!settings()->useThreadedCompositor());
    m_client->scheduleComposite();
}

void WebViewImpl::updateLayerTreeViewport()
{
    if (!page() || !m_nonCompositedContentHost || !m_layerTreeHost)
        return;

    FrameView* view = page()->mainFrame()->view();
    IntRect visibleRect = view->visibleContentRect(true /* include scrollbars */);
    IntPoint scroll(view->scrollX(), view->scrollY());

    m_nonCompositedContentHost->setViewport(visibleRect.size(), view->contentsSize(), scroll, pageScaleFactor());
    m_layerTreeHost->setViewport(visibleRect.size());
    m_layerTreeHost->setPageScale(pageScaleFactor());
}

WebGraphicsContext3D* WebViewImpl::graphicsContext3D()
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_page->settings()->acceleratedCompositingEnabled() && allowsAcceleratedCompositing()) {
        if (m_layerTreeHost) {
            WebGraphicsContext3D* webContext = GraphicsContext3DPrivate::extractWebGraphicsContext3D(m_layerTreeHost->context());
            if (webContext && !webContext->isContextLost())
                return webContext;
        }
        if (m_temporaryOnscreenGraphicsContext3D) {
            WebGraphicsContext3D* webContext = GraphicsContext3DPrivate::extractWebGraphicsContext3D(m_temporaryOnscreenGraphicsContext3D.get());
            if (webContext && !webContext->isContextLost())
                return webContext;
        }
        if (settings()->useThreadedCompositor())
            m_temporaryOnscreenGraphicsContext3D = GraphicsContext3DPrivate::createGraphicsContextForAnotherThread(getCompositorContextAttributes(), m_page->chrome(), GraphicsContext3D::RenderDirectlyToHostWindow);
        else
            m_temporaryOnscreenGraphicsContext3D = GraphicsContext3D::create(getCompositorContextAttributes(), m_page->chrome(), GraphicsContext3D::RenderDirectlyToHostWindow);

        return GraphicsContext3DPrivate::extractWebGraphicsContext3D(m_temporaryOnscreenGraphicsContext3D.get());
    }
#endif
    return 0;
}


void WebViewImpl::setVisibilityState(WebPageVisibilityState visibilityState,
                                     bool isInitialState) {
    if (!page())
        return;

#if ENABLE(PAGE_VISIBILITY_API)
    ASSERT(visibilityState == WebPageVisibilityStateVisible
           || visibilityState == WebPageVisibilityStateHidden
           || visibilityState == WebPageVisibilityStatePrerender);
    m_page->setVisibilityState(static_cast<PageVisibilityState>(static_cast<int>(visibilityState)), isInitialState);
#endif

#if USE(ACCELERATED_COMPOSITING)
    if (isAcceleratedCompositingActive()) {
        bool visible = visibilityState == WebPageVisibilityStateVisible;
        if (!visible)
            m_nonCompositedContentHost->protectVisibleTileTextures();
        m_layerTreeHost->setVisible(visible);
    }
#endif
}

#if ENABLE(GESTURE_RECOGNIZER)
void WebViewImpl::resetGestureRecognizer()
{
    m_gestureRecognizer->reset();
}
#endif

} // namespace WebKit
