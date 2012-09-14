/*
 * Copyright (C) 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DOMWindow.h"

#include "AbstractDatabase.h"
#include "BackForwardController.h"
#include "BarInfo.h"
#include "BeforeUnloadEvent.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSRuleList.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Console.h"
#include "Crypto.h"
#include "DOMApplicationCache.h"
#include "DOMSelection.h"
#include "DOMSettableTokenList.h"
#include "DOMStringList.h"
#include "DOMTimer.h"
#include "DOMTokenList.h"
#include "DOMURL.h"
#include "DOMWindowExtension.h"
#include "DOMWindowNotifications.h"
#include "Database.h"
#include "DatabaseCallback.h"
#include "DeviceMotionController.h"
#include "DeviceOrientationController.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Element.h"
#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "History.h"
#include "InspectorInstrumentation.h"
#include "KURL.h"
#include "Location.h"
#include "MediaQueryList.h"
#include "MediaQueryMatcher.h"
#include "MessageEvent.h"
#include "Navigator.h"
#include "Page.h"
#include "PageGroup.h"
#include "PageTransitionEvent.h"
#include "Performance.h"
#include "PlatformScreen.h"
#include "RuntimeEnabledFeatures.h"
#include "ScheduledAction.h"
#include "Screen.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "SecurityOrigin.h"
#include "SerializedScriptValue.h"
#include "Settings.h"
#include "Storage.h"
#include "StorageArea.h"
#include "StorageNamespace.h"
#include "StyleMedia.h"
#include "StyleResolver.h"
#include "SuddenTermination.h"
#include "WebKitPoint.h"
#include "WindowFeatures.h"
#include "WindowFocusAllowedIndicator.h"
#include <algorithm>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/MathExtras.h>
#include <wtf/text/Base64.h>
#include <wtf/text/WTFString.h>

#if ENABLE(REQUEST_ANIMATION_FRAME)
#include "RequestAnimationFrameCallback.h"
#endif

using std::min;
using std::max;

namespace WebCore {

class PostMessageTimer : public TimerBase {
public:
    PostMessageTimer(DOMWindow* window, PassRefPtr<SerializedScriptValue> message, const String& sourceOrigin, PassRefPtr<DOMWindow> source, PassOwnPtr<MessagePortChannelArray> channels, SecurityOrigin* targetOrigin, PassRefPtr<ScriptCallStack> stackTrace)
        : m_window(window)
        , m_message(message)
        , m_origin(sourceOrigin)
        , m_source(source)
        , m_channels(channels)
        , m_targetOrigin(targetOrigin)
        , m_stackTrace(stackTrace)
    {
    }

    PassRefPtr<MessageEvent> event(ScriptExecutionContext* context)
    {
        OwnPtr<MessagePortArray> messagePorts = MessagePort::entanglePorts(*context, m_channels.release());
        return MessageEvent::create(messagePorts.release(), m_message, m_origin, "", m_source);
    }
    SecurityOrigin* targetOrigin() const { return m_targetOrigin.get(); }
    ScriptCallStack* stackTrace() const { return m_stackTrace.get(); }

private:
    virtual void fired()
    {
        m_window->postMessageTimerFired(adoptPtr(this));
        // This object is deleted now.
    }

    RefPtr<DOMWindow> m_window;
    RefPtr<SerializedScriptValue> m_message;
    String m_origin;
    RefPtr<DOMWindow> m_source;
    OwnPtr<MessagePortChannelArray> m_channels;
    RefPtr<SecurityOrigin> m_targetOrigin;
    RefPtr<ScriptCallStack> m_stackTrace;
};

typedef HashCountedSet<DOMWindow*> DOMWindowSet;

static DOMWindowSet& windowsWithUnloadEventListeners()
{
    DEFINE_STATIC_LOCAL(DOMWindowSet, windowsWithUnloadEventListeners, ());
    return windowsWithUnloadEventListeners;
}

static DOMWindowSet& windowsWithBeforeUnloadEventListeners()
{
    DEFINE_STATIC_LOCAL(DOMWindowSet, windowsWithBeforeUnloadEventListeners, ());
    return windowsWithBeforeUnloadEventListeners;
}

static void addUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    if (set.isEmpty())
        disableSuddenTermination();
    set.add(domWindow);
}

static void removeUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.remove(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static void removeAllUnloadEventListeners(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.removeAll(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static void addBeforeUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    if (set.isEmpty())
        disableSuddenTermination();
    set.add(domWindow);
}

static void removeBeforeUnloadEventListener(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.remove(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static void removeAllBeforeUnloadEventListeners(DOMWindow* domWindow)
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    DOMWindowSet::iterator it = set.find(domWindow);
    if (it == set.end())
        return;
    set.removeAll(it);
    if (set.isEmpty())
        enableSuddenTermination();
}

static bool allowsBeforeUnloadListeners(DOMWindow* window)
{
    ASSERT_ARG(window, window);
    Frame* frame = window->frame();
    if (!frame)
        return false;
    Page* page = frame->page();
    if (!page)
        return false;
    return frame == page->mainFrame();
}

bool DOMWindow::dispatchAllPendingBeforeUnloadEvents()
{
    DOMWindowSet& set = windowsWithBeforeUnloadEventListeners();
    if (set.isEmpty())
        return true;

    static bool alreadyDispatched = false;
    ASSERT(!alreadyDispatched);
    if (alreadyDispatched)
        return true;

    Vector<RefPtr<DOMWindow> > windows;
    DOMWindowSet::iterator end = set.end();
    for (DOMWindowSet::iterator it = set.begin(); it != end; ++it)
        windows.append(it->first);

    size_t size = windows.size();
    for (size_t i = 0; i < size; ++i) {
        DOMWindow* window = windows[i].get();
        if (!set.contains(window))
            continue;

        Frame* frame = window->frame();
        if (!frame)
            continue;

        if (!frame->loader()->shouldClose())
            return false;
    }

    enableSuddenTermination();

    alreadyDispatched = true;

    return true;
}

unsigned DOMWindow::pendingUnloadEventListeners() const
{
    return windowsWithUnloadEventListeners().count(const_cast<DOMWindow*>(this));
}

void DOMWindow::dispatchAllPendingUnloadEvents()
{
    DOMWindowSet& set = windowsWithUnloadEventListeners();
    if (set.isEmpty())
        return;

    static bool alreadyDispatched = false;
    ASSERT(!alreadyDispatched);
    if (alreadyDispatched)
        return;

    Vector<RefPtr<DOMWindow> > windows;
    DOMWindowSet::iterator end = set.end();
    for (DOMWindowSet::iterator it = set.begin(); it != end; ++it)
        windows.append(it->first);

    size_t size = windows.size();
    for (size_t i = 0; i < size; ++i) {
        DOMWindow* window = windows[i].get();
        if (!set.contains(window))
            continue;

        window->dispatchEvent(PageTransitionEvent::create(eventNames().pagehideEvent, false), window->document());
        window->dispatchEvent(Event::create(eventNames().unloadEvent, false, false), window->document());
    }

    enableSuddenTermination();

    alreadyDispatched = true;
}

// This function:
// 1) Validates the pending changes are not changing to NaN
// 2) Constrains the window rect to the minimum window size and no bigger than
//    the the float rect's dimensions.
// 3) Constrain window rect to within the top and left boundaries of the screen rect
// 4) Constraint the window rect to within the bottom and right boundaries of the
//    screen rect.
// 5) Translate the window rect coordinates to be within the coordinate space of
//    the screen rect.
void DOMWindow::adjustWindowRect(const FloatRect& screen, FloatRect& window, const FloatRect& pendingChanges) const
{
    // Make sure we're in a valid state before adjusting dimensions.
    ASSERT(isfinite(screen.x()));
    ASSERT(isfinite(screen.y()));
    ASSERT(isfinite(screen.width()));
    ASSERT(isfinite(screen.height()));
    ASSERT(isfinite(window.x()));
    ASSERT(isfinite(window.y()));
    ASSERT(isfinite(window.width()));
    ASSERT(isfinite(window.height()));
    
    // Update window values if new requested values are not NaN.
    if (!isnan(pendingChanges.x()))
        window.setX(pendingChanges.x());
    if (!isnan(pendingChanges.y()))
        window.setY(pendingChanges.y());
    if (!isnan(pendingChanges.width()))
        window.setWidth(pendingChanges.width());
    if (!isnan(pendingChanges.height()))
        window.setHeight(pendingChanges.height());

    ASSERT(m_frame);
    Page* page = m_frame->page();
    FloatSize minimumSize = page ? m_frame->page()->chrome()->client()->minimumWindowSize() : FloatSize(100, 100);
    window.setWidth(min(max(minimumSize.width(), window.width()), screen.width()));
    window.setHeight(min(max(minimumSize.height(), window.height()), screen.height()));

    // Constrain the window position to the screen.
    window.setX(max(screen.x(), min(window.x(), screen.maxX() - window.width())));
    window.setY(max(screen.y(), min(window.y(), screen.maxY() - window.height())));
}

// FIXME: We can remove this function once V8 showModalDialog is changed to use DOMWindow.
void DOMWindow::parseModalDialogFeatures(const String& string, HashMap<String, String>& map)
{
    WindowFeatures::parseDialogFeatures(string, map);
}

bool DOMWindow::allowPopUp(Frame* firstFrame)
{
    ASSERT(firstFrame);

    if (ScriptController::processingUserGesture())
        return true;

    Settings* settings = firstFrame->settings();
    return settings && settings->javaScriptCanOpenWindowsAutomatically();
}

bool DOMWindow::allowPopUp()
{
    return m_frame && allowPopUp(m_frame);
}

bool DOMWindow::canShowModalDialog(const Frame* frame)
{
    if (!frame)
        return false;
    Page* page = frame->page();
    if (!page)
        return false;
    return page->chrome()->canRunModal();
}

bool DOMWindow::canShowModalDialogNow(const Frame* frame)
{
    if (!frame)
        return false;
    Page* page = frame->page();
    if (!page)
        return false;
    return page->chrome()->canRunModalNow();
}

DOMWindow::DOMWindow(Document* document)
    : ContextDestructionObserver(document)
    , FrameDestructionObserver(document->frame())
    , m_shouldPrintWhenFinishedLoading(false)
    , m_suspendedForPageCache(false)
{
    ASSERT(frame());
    ASSERT(DOMWindow::document());
}

void DOMWindow::didSecureTransitionTo(Document* document)
{
    observeContext(document);
}

DOMWindow::~DOMWindow()
{
#ifndef NDEBUG
    if (!m_suspendedForPageCache) {
        ASSERT(!m_screen);
        ASSERT(!m_history);
        ASSERT(!m_crypto);
        ASSERT(!m_locationbar);
        ASSERT(!m_menubar);
        ASSERT(!m_personalbar);
        ASSERT(!m_scrollbars);
        ASSERT(!m_statusbar);
        ASSERT(!m_toolbar);
        ASSERT(!m_console);
        ASSERT(!m_navigator);
#if ENABLE(WEB_TIMING)
        ASSERT(!m_performance);
#endif
        ASSERT(!m_location);
        ASSERT(!m_media);
        ASSERT(!m_sessionStorage);
        ASSERT(!m_localStorage);
        ASSERT(!m_applicationCache);
    }
#endif

    if (m_suspendedForPageCache)
        willDestroyCachedFrame();
    else
        willDestroyDocumentInFrame();

    // As the ASSERTs above indicate, this reset should only be necessary if this DOMWindow is suspended for the page cache.
    // But we don't want to risk any of these objects hanging around after we've been destroyed.
    resetDOMWindowProperties();

    removeAllUnloadEventListeners(this);
    removeAllBeforeUnloadEventListeners(this);
}

const AtomicString& DOMWindow::interfaceName() const
{
    return eventNames().interfaceForDOMWindow;
}

ScriptExecutionContext* DOMWindow::scriptExecutionContext() const
{
    return ContextDestructionObserver::scriptExecutionContext();
}

DOMWindow* DOMWindow::toDOMWindow()
{
    return this;
}

PassRefPtr<MediaQueryList> DOMWindow::matchMedia(const String& media)
{
    return document() ? document()->mediaQueryMatcher()->matchMedia(media) : 0;
}

Page* DOMWindow::page()
{
    return frame() ? frame()->page() : 0;
}

void DOMWindow::frameDestroyed()
{
    willDestroyDocumentInFrame();
    FrameDestructionObserver::frameDestroyed();
    resetDOMWindowProperties();
}

void DOMWindow::willDetachPage()
{
    InspectorInstrumentation::frameWindowDiscarded(m_frame, this);
}

void DOMWindow::willDestroyCachedFrame()
{
    // It is necessary to copy m_properties to a separate vector because the DOMWindowProperties may
    // unregister themselves from the DOMWindow as a result of the call to willDestroyGlobalObjectInCachedFrame.
    Vector<DOMWindowProperty*> properties;
    copyToVector(m_properties, properties);
    for (size_t i = 0; i < properties.size(); ++i)
        properties[i]->willDestroyGlobalObjectInCachedFrame();
}

void DOMWindow::willDestroyDocumentInFrame()
{
    // It is necessary to copy m_properties to a separate vector because the DOMWindowProperties may
    // unregister themselves from the DOMWindow as a result of the call to willDestroyGlobalObjectInFrame.
    Vector<DOMWindowProperty*> properties;
    copyToVector(m_properties, properties);
    for (size_t i = 0; i < properties.size(); ++i)
        properties[i]->willDestroyGlobalObjectInFrame();
}

void DOMWindow::willDetachDocumentFromFrame()
{
    // It is necessary to copy m_properties to a separate vector because the DOMWindowProperties may
    // unregister themselves from the DOMWindow as a result of the call to willDetachGlobalObjectFromFrame.
    Vector<DOMWindowProperty*> properties;
    copyToVector(m_properties, properties);
    for (size_t i = 0; i < properties.size(); ++i)
        properties[i]->willDetachGlobalObjectFromFrame();
}

void DOMWindow::registerProperty(DOMWindowProperty* property)
{
    m_properties.add(property);
}

void DOMWindow::unregisterProperty(DOMWindowProperty* property)
{
    m_properties.remove(property);
}

void DOMWindow::resetUnlessSuspendedForPageCache()
{
    if (m_suspendedForPageCache)
        return;
    willDestroyDocumentInFrame();
    resetDOMWindowProperties();
}

void DOMWindow::suspendForPageCache()
{
    disconnectDOMWindowProperties();
    m_suspendedForPageCache = true;
}

void DOMWindow::resumeFromPageCache()
{
    reconnectDOMWindowProperties();
    m_suspendedForPageCache = false;
}

void DOMWindow::disconnectDOMWindowProperties()
{
    // It is necessary to copy m_properties to a separate vector because the DOMWindowProperties may
    // unregister themselves from the DOMWindow as a result of the call to disconnectFrameForPageCache.
    Vector<DOMWindowProperty*> properties;
    copyToVector(m_properties, properties);
    for (size_t i = 0; i < properties.size(); ++i)
        properties[i]->disconnectFrameForPageCache();
}

void DOMWindow::reconnectDOMWindowProperties()
{
    ASSERT(m_suspendedForPageCache);
    // It is necessary to copy m_properties to a separate vector because the DOMWindowProperties may
    // unregister themselves from the DOMWindow as a result of the call to reconnectFromPageCache.
    Vector<DOMWindowProperty*> properties;
    copyToVector(m_properties, properties);
    for (size_t i = 0; i < properties.size(); ++i)
        properties[i]->reconnectFrameFromPageCache(m_frame);
}

void DOMWindow::resetDOMWindowProperties()
{
    m_properties.clear();

    m_screen = 0;
    m_history = 0;
    m_crypto = 0;
    m_locationbar = 0;
    m_menubar = 0;
    m_personalbar = 0;
    m_scrollbars = 0;
    m_statusbar = 0;
    m_toolbar = 0;
    m_console = 0;
    m_navigator = 0;
#if ENABLE(WEB_TIMING)
    m_performance = 0;
#endif
    m_location = 0;
    m_media = 0;
    m_sessionStorage = 0;
    m_localStorage = 0;
    m_applicationCache = 0;
}

bool DOMWindow::isCurrentlyDisplayedInFrame() const
{
    return m_frame && m_frame->document()->domWindow() == this;
}

#if ENABLE(ORIENTATION_EVENTS)
int DOMWindow::orientation() const
{
    if (!m_frame)
        return 0;

    return m_frame->orientation();
}
#endif

Screen* DOMWindow::screen() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_screen)
        m_screen = Screen::create(m_frame);
    return m_screen.get();
}

History* DOMWindow::history() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_history)
        m_history = History::create(m_frame);
    return m_history.get();
}

Crypto* DOMWindow::crypto() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_crypto)
        m_crypto = Crypto::create();
    return m_crypto.get();
}

BarInfo* DOMWindow::locationbar() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_locationbar)
        m_locationbar = BarInfo::create(m_frame, BarInfo::Locationbar);
    return m_locationbar.get();
}

BarInfo* DOMWindow::menubar() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_menubar)
        m_menubar = BarInfo::create(m_frame, BarInfo::Menubar);
    return m_menubar.get();
}

BarInfo* DOMWindow::personalbar() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_personalbar)
        m_personalbar = BarInfo::create(m_frame, BarInfo::Personalbar);
    return m_personalbar.get();
}

BarInfo* DOMWindow::scrollbars() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_scrollbars)
        m_scrollbars = BarInfo::create(m_frame, BarInfo::Scrollbars);
    return m_scrollbars.get();
}

BarInfo* DOMWindow::statusbar() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_statusbar)
        m_statusbar = BarInfo::create(m_frame, BarInfo::Statusbar);
    return m_statusbar.get();
}

BarInfo* DOMWindow::toolbar() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_toolbar)
        m_toolbar = BarInfo::create(m_frame, BarInfo::Toolbar);
    return m_toolbar.get();
}

Console* DOMWindow::console() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_console)
        m_console = Console::create(m_frame);
    return m_console.get();
}

DOMApplicationCache* DOMWindow::applicationCache() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_applicationCache)
        m_applicationCache = DOMApplicationCache::create(m_frame);
    return m_applicationCache.get();
}

Navigator* DOMWindow::navigator() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_navigator)
        m_navigator = Navigator::create(m_frame);
    return m_navigator.get();
}

#if ENABLE(WEB_TIMING)
Performance* DOMWindow::performance() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_performance)
        m_performance = Performance::create(m_frame);
    return m_performance.get();
}
#endif

Location* DOMWindow::location() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_location)
        m_location = Location::create(m_frame);
    return m_location.get();
}

Storage* DOMWindow::sessionStorage(ExceptionCode& ec) const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;

    Document* document = this->document();
    if (!document)
        return 0;

    if (!document->securityOrigin()->canAccessLocalStorage(document->topDocument()->securityOrigin())) {
        ec = SECURITY_ERR;
        return 0;
    }

    if (m_sessionStorage)
        return m_sessionStorage.get();

    Page* page = document->page();
    if (!page)
        return 0;

    RefPtr<StorageArea> storageArea = page->sessionStorage()->storageArea(document->securityOrigin());
    InspectorInstrumentation::didUseDOMStorage(page, storageArea.get(), false, m_frame);

    m_sessionStorage = Storage::create(m_frame, storageArea.release());
    return m_sessionStorage.get();
}

Storage* DOMWindow::localStorage(ExceptionCode& ec) const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;

    Document* document = this->document();
    if (!document)
        return 0;

    if (!document->securityOrigin()->canAccessLocalStorage(document->topDocument()->securityOrigin())) {
        ec = SECURITY_ERR;
        return 0;
    }

    if (m_localStorage)
        return m_localStorage.get();

    Page* page = document->page();
    if (!page)
        return 0;

    if (!page->settings()->localStorageEnabled())
        return 0;

    RefPtr<StorageArea> storageArea = page->group().localStorage()->storageArea(document->securityOrigin());
    InspectorInstrumentation::didUseDOMStorage(page, storageArea.get(), true, m_frame);

    m_localStorage = Storage::create(m_frame, storageArea.release());
    return m_localStorage.get();
}

void DOMWindow::postMessage(PassRefPtr<SerializedScriptValue> message, MessagePort* port, const String& targetOrigin, DOMWindow* source, ExceptionCode& ec)
{
    MessagePortArray ports;
    if (port)
        ports.append(port);
    postMessage(message, &ports, targetOrigin, source, ec);
}

void DOMWindow::postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray* ports, const String& targetOrigin, DOMWindow* source, ExceptionCode& ec)
{
    if (!isCurrentlyDisplayedInFrame())
        return;

    Document* sourceDocument = source->document();

    // Compute the target origin.  We need to do this synchronously in order
    // to generate the SYNTAX_ERR exception correctly.
    RefPtr<SecurityOrigin> target;
    if (targetOrigin == "/") {
        if (!sourceDocument)
            return;
        target = sourceDocument->securityOrigin();
    } else if (targetOrigin != "*") {
        target = SecurityOrigin::createFromString(targetOrigin);
        // It doesn't make sense target a postMessage at a unique origin
        // because there's no way to represent a unique origin in a string.
        if (target->isUnique()) {
            ec = SYNTAX_ERR;
            return;
        }
    }

    OwnPtr<MessagePortChannelArray> channels = MessagePort::disentanglePorts(ports, ec);
    if (ec)
        return;

    // Capture the source of the message.  We need to do this synchronously
    // in order to capture the source of the message correctly.
    if (!sourceDocument)
        return;
    String sourceOrigin = sourceDocument->securityOrigin()->toString();

    // Capture stack trace only when inspector front-end is loaded as it may be time consuming.
    RefPtr<ScriptCallStack> stackTrace;
    if (InspectorInstrumentation::hasFrontends())
        stackTrace = createScriptCallStack(ScriptCallStack::maxCallStackSizeToCapture, true);

    // Schedule the message.
    PostMessageTimer* timer = new PostMessageTimer(this, message, sourceOrigin, source, channels.release(), target.get(), stackTrace.release());
    timer->startOneShot(0);
}

void DOMWindow::postMessageTimerFired(PassOwnPtr<PostMessageTimer> t)
{
    OwnPtr<PostMessageTimer> timer(t);

    if (!document() || !isCurrentlyDisplayedInFrame())
        return;

    RefPtr<MessageEvent> event = timer->event(document());

    // Give the embedder a chance to intercept this postMessage because this
    // DOMWindow might be a proxy for another in browsers that support
    // postMessage calls across WebKit instances.
    if (m_frame->loader()->client()->willCheckAndDispatchMessageEvent(timer->targetOrigin(), event.get()))
        return;

    dispatchMessageEventWithOriginCheck(timer->targetOrigin(), event, timer->stackTrace());
}

void DOMWindow::dispatchMessageEventWithOriginCheck(SecurityOrigin* intendedTargetOrigin, PassRefPtr<Event> event, PassRefPtr<ScriptCallStack> stackTrace)
{
    if (intendedTargetOrigin) {
        // Check target origin now since the target document may have changed since the timer was scheduled.
        if (!intendedTargetOrigin->isSameSchemeHostPort(document()->securityOrigin())) {
            String message = "Unable to post message to " + intendedTargetOrigin->toString() +
                             ". Recipient has origin " + document()->securityOrigin()->toString() + ".\n";
            console()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, stackTrace);
            return;
        }
    }

    dispatchEvent(event);
}

DOMSelection* DOMWindow::getSelection()
{
    if (!isCurrentlyDisplayedInFrame() || !m_frame)
        return 0;

    return m_frame->document()->getSelection();
}

Element* DOMWindow::frameElement() const
{
    if (!m_frame)
        return 0;

    return m_frame->ownerElement();
}

void DOMWindow::focus(ScriptExecutionContext* context)
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    bool allowFocus = WindowFocusAllowedIndicator::windowFocusAllowed() || !m_frame->settings()->windowFocusRestricted();
    if (context) {
        ASSERT(isMainThread());
        Document* activeDocument = static_cast<Document*>(context);
        if (opener() && activeDocument->domWindow() == opener())
            allowFocus = true;
    }

    // If we're a top level window, bring the window to the front.
    if (m_frame == page->mainFrame() && allowFocus)
        page->chrome()->focus();

    if (!m_frame)
        return;

    m_frame->eventHandler()->focusDocumentView();
}

void DOMWindow::blur()
{

    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame->settings()->windowFocusRestricted())
        return;

    if (m_frame != page->mainFrame())
        return;

    page->chrome()->unfocus();
}

void DOMWindow::close(ScriptExecutionContext* context)
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    if (context) {
        ASSERT(isMainThread());
        Document* activeDocument = static_cast<Document*>(context);
        if (!activeDocument)
            return;

        if (!activeDocument->canNavigate(m_frame))
            return;
    }

    Settings* settings = m_frame->settings();
    bool allowScriptsToCloseWindows = settings && settings->allowScriptsToCloseWindows();

    if (!(page->openedByDOM() || page->backForward()->count() <= 1 || allowScriptsToCloseWindows))
        return;

    if (!m_frame->loader()->shouldClose())
        return;

    page->chrome()->closeWindowSoon();
}

void DOMWindow::print()
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame->loader()->activeDocumentLoader()->isLoading()) {
        m_shouldPrintWhenFinishedLoading = true;
        return;
    }
    m_shouldPrintWhenFinishedLoading = false;
    page->chrome()->print(m_frame);
}

void DOMWindow::stop()
{
    if (!m_frame)
        return;

    // We must check whether the load is complete asynchronously, because we might still be parsing
    // the document until the callstack unwinds.
    m_frame->loader()->stopForUserCancel(true);
}

void DOMWindow::alert(const String& message)
{
    if (!m_frame)
        return;

    m_frame->document()->updateStyleIfNeeded();

    Page* page = m_frame->page();
    if (!page)
        return;

    page->chrome()->runJavaScriptAlert(m_frame, message);
}

bool DOMWindow::confirm(const String& message)
{
    if (!m_frame)
        return false;

    m_frame->document()->updateStyleIfNeeded();

    Page* page = m_frame->page();
    if (!page)
        return false;

    return page->chrome()->runJavaScriptConfirm(m_frame, message);
}

String DOMWindow::prompt(const String& message, const String& defaultValue)
{
    if (!m_frame)
        return String();

    m_frame->document()->updateStyleIfNeeded();

    Page* page = m_frame->page();
    if (!page)
        return String();

    String returnValue;
    if (page->chrome()->runJavaScriptPrompt(m_frame, message, defaultValue, returnValue))
        return returnValue;

    return String();
}

String DOMWindow::btoa(const String& stringToEncode, ExceptionCode& ec)
{
    if (stringToEncode.isNull())
        return String();

    if (!stringToEncode.containsOnlyLatin1()) {
        ec = INVALID_CHARACTER_ERR;
        return String();
    }

    return base64Encode(stringToEncode.latin1());
}

String DOMWindow::atob(const String& encodedString, ExceptionCode& ec)
{
    if (encodedString.isNull())
        return String();

    if (!encodedString.containsOnlyLatin1()) {
        ec = INVALID_CHARACTER_ERR;
        return String();
    }

    Vector<char> out;
    if (!base64Decode(encodedString, out, Base64FailOnInvalidCharacter)) {
        ec = INVALID_CHARACTER_ERR;
        return String();
    }

    return String(out.data(), out.size());
}

bool DOMWindow::find(const String& string, bool caseSensitive, bool backwards, bool wrap, bool /*wholeWord*/, bool /*searchInFrames*/, bool /*showDialog*/) const
{
    if (!isCurrentlyDisplayedInFrame())
        return false;

    // FIXME (13016): Support wholeWord, searchInFrames and showDialog
    return m_frame->editor()->findString(string, !backwards, caseSensitive, wrap, false);
}

bool DOMWindow::offscreenBuffering() const
{
    return true;
}

int DOMWindow::outerHeight() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().height());
}

int DOMWindow::outerWidth() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().width());
}

int DOMWindow::innerHeight() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;

    // If the device height is overridden, do not include the horizontal scrollbar into the innerHeight (since it is absent on the real device).
    bool includeScrollbars = !InspectorInstrumentation::shouldApplyScreenHeightOverride(m_frame);
    return view->mapFromLayoutToCSSUnits(static_cast<int>(view->visibleContentRect(includeScrollbars).height()));
}

int DOMWindow::innerWidth() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;

    // If the device width is overridden, do not include the vertical scrollbar into the innerWidth (since it is absent on the real device).
    bool includeScrollbars = !InspectorInstrumentation::shouldApplyScreenWidthOverride(m_frame);
    return view->mapFromLayoutToCSSUnits(static_cast<int>(view->visibleContentRect(includeScrollbars).width()));
}

int DOMWindow::screenX() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().x());
}

int DOMWindow::screenY() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return static_cast<int>(page->chrome()->windowRect().y());
}

int DOMWindow::scrollX() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    return view->mapFromLayoutToCSSUnits(view->scrollX());
}

int DOMWindow::scrollY() const
{
    if (!m_frame)
        return 0;

    FrameView* view = m_frame->view();
    if (!view)
        return 0;

    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    return view->mapFromLayoutToCSSUnits(view->scrollY());
}

bool DOMWindow::closed() const
{
    return !m_frame;
}

unsigned DOMWindow::length() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;

    return m_frame->tree()->scopedChildCount();
}

String DOMWindow::name() const
{
    if (!m_frame)
        return String();

    return m_frame->tree()->name();
}

void DOMWindow::setName(const String& string)
{
    if (!m_frame)
        return;

    m_frame->tree()->setName(string);
}

void DOMWindow::setStatus(const String& string) 
{
    m_status = string;

    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    ASSERT(m_frame->document()); // Client calls shouldn't be made when the frame is in inconsistent state.
    page->chrome()->setStatusbarText(m_frame, m_status);
} 
    
void DOMWindow::setDefaultStatus(const String& string) 
{
    m_defaultStatus = string;

    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    ASSERT(m_frame->document()); // Client calls shouldn't be made when the frame is in inconsistent state.
    page->chrome()->setStatusbarText(m_frame, m_defaultStatus);
}

DOMWindow* DOMWindow::self() const
{
    if (!m_frame)
        return 0;

    return m_frame->document()->domWindow();
}

DOMWindow* DOMWindow::opener() const
{
    if (!m_frame)
        return 0;

    Frame* opener = m_frame->loader()->opener();
    if (!opener)
        return 0;

    return opener->document()->domWindow();
}

DOMWindow* DOMWindow::parent() const
{
    if (!m_frame)
        return 0;

    Frame* parent = m_frame->tree()->parent();
    if (parent)
        return parent->document()->domWindow();

    return m_frame->document()->domWindow();
}

DOMWindow* DOMWindow::top() const
{
    if (!m_frame)
        return 0;

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return m_frame->tree()->top()->document()->domWindow();
}

Document* DOMWindow::document() const
{
    ScriptExecutionContext* context = ContextDestructionObserver::scriptExecutionContext();
    ASSERT(!context || context->isDocument());
    return static_cast<Document*>(context);
}

PassRefPtr<StyleMedia> DOMWindow::styleMedia() const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    if (!m_media)
        m_media = StyleMedia::create(m_frame);
    return m_media.get();
}

PassRefPtr<CSSStyleDeclaration> DOMWindow::getComputedStyle(Element* elt, const String& pseudoElt) const
{
    if (!elt)
        return 0;

    return CSSComputedStyleDeclaration::create(elt, false, pseudoElt);
}

PassRefPtr<CSSRuleList> DOMWindow::getMatchedCSSRules(Element* element, const String& pseudoElement, bool authorOnly) const
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;

    unsigned colonStart = pseudoElement[0] == ':' ? (pseudoElement[1] == ':' ? 2 : 1) : 0;
    CSSSelector::PseudoType pseudoType = CSSSelector::parsePseudoType(AtomicString(pseudoElement.substring(colonStart)));
    if (pseudoType == CSSSelector::PseudoUnknown && !pseudoElement.isEmpty())
        return 0;

    unsigned rulesToInclude = StyleResolver::AuthorCSSRules;
    if (!authorOnly)
        rulesToInclude |= StyleResolver::UAAndUserCSSRules;
    if (Settings* settings = m_frame->settings()) {
        if (settings->crossOriginCheckInGetMatchedCSSRulesDisabled())
            rulesToInclude |= StyleResolver::CrossOriginCSSRules;
    }

    PseudoId pseudoId = CSSSelector::pseudoId(pseudoType);

    return m_frame->document()->styleResolver()->pseudoStyleRulesForElement(element, pseudoId, rulesToInclude);
}

PassRefPtr<WebKitPoint> DOMWindow::webkitConvertPointFromNodeToPage(Node* node, const WebKitPoint* p) const
{
    if (!node || !p)
        return 0;

    if (!document())
        return 0;

    document()->updateLayoutIgnorePendingStylesheets();

    FloatPoint pagePoint(p->x(), p->y());
    pagePoint = node->convertToPage(pagePoint);
    return WebKitPoint::create(pagePoint.x(), pagePoint.y());
}

PassRefPtr<WebKitPoint> DOMWindow::webkitConvertPointFromPageToNode(Node* node, const WebKitPoint* p) const
{
    if (!node || !p)
        return 0;

    if (!document())
        return 0;

    document()->updateLayoutIgnorePendingStylesheets();

    FloatPoint nodePoint(p->x(), p->y());
    nodePoint = node->convertFromPage(nodePoint);
    return WebKitPoint::create(nodePoint.x(), nodePoint.y());
}

double DOMWindow::devicePixelRatio() const
{
    if (!m_frame)
        return 0.0;

    Page* page = m_frame->page();
    if (!page)
        return 0.0;

    return page->deviceScaleFactor();
}

void DOMWindow::scrollBy(int x, int y) const
{
    if (!isCurrentlyDisplayedInFrame())
        return;

    document()->updateLayoutIgnorePendingStylesheets();

    FrameView* view = m_frame->view();
    if (!view)
        return;

    IntSize scaledOffset(view->mapFromCSSToLayoutUnits(x), view->mapFromCSSToLayoutUnits(y));
    view->scrollBy(scaledOffset);
}

void DOMWindow::scrollTo(int x, int y) const
{
    if (!isCurrentlyDisplayedInFrame())
        return;

    document()->updateLayoutIgnorePendingStylesheets();

    RefPtr<FrameView> view = m_frame->view();
    if (!view)
        return;

    IntPoint layoutPos(view->mapFromCSSToLayoutUnits(x), view->mapFromCSSToLayoutUnits(y));
    view->setScrollPosition(layoutPos);
}

void DOMWindow::moveBy(float x, float y) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatRect update = fr;
    update.move(x, y);
    // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
    adjustWindowRect(screenAvailableRect(page->mainFrame()->view()), fr, update);
    page->chrome()->setWindowRect(fr);
}

void DOMWindow::moveTo(float x, float y) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatRect sr = screenAvailableRect(page->mainFrame()->view());
    fr.setLocation(sr.location());
    FloatRect update = fr;
    update.move(x, y);     
    // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
    adjustWindowRect(sr, fr, update);
    page->chrome()->setWindowRect(fr);
}

void DOMWindow::resizeBy(float x, float y) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatSize dest = fr.size() + FloatSize(x, y);
    FloatRect update(fr.location(), dest);
    adjustWindowRect(screenAvailableRect(page->mainFrame()->view()), fr, update);
    page->chrome()->setWindowRect(fr);
}

void DOMWindow::resizeTo(float width, float height) const
{
    if (!m_frame)
        return;

    Page* page = m_frame->page();
    if (!page)
        return;

    if (m_frame != page->mainFrame())
        return;

    FloatRect fr = page->chrome()->windowRect();
    FloatSize dest = FloatSize(width, height);
    FloatRect update(fr.location(), dest);
    adjustWindowRect(screenAvailableRect(page->mainFrame()->view()), fr, update);
    page->chrome()->setWindowRect(fr);
}

int DOMWindow::setTimeout(PassOwnPtr<ScheduledAction> action, int timeout, ExceptionCode& ec)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context) {
        ec = INVALID_ACCESS_ERR;
        return -1;
    }
    return DOMTimer::install(context, action, timeout, true);
}

void DOMWindow::clearTimeout(int timeoutId)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context)
        return;
    DOMTimer::removeById(context, timeoutId);
}

int DOMWindow::setInterval(PassOwnPtr<ScheduledAction> action, int timeout, ExceptionCode& ec)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context) {
        ec = INVALID_ACCESS_ERR;
        return -1;
    }
    return DOMTimer::install(context, action, timeout, false);
}

void DOMWindow::clearInterval(int timeoutId)
{
    ScriptExecutionContext* context = scriptExecutionContext();
    if (!context)
        return;
    DOMTimer::removeById(context, timeoutId);
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
int DOMWindow::webkitRequestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback> callback)
{
    if (Document* d = document())
        return d->webkitRequestAnimationFrame(callback);
    return 0;
}

void DOMWindow::webkitCancelAnimationFrame(int id)
{
    if (Document* d = document())
        d->webkitCancelAnimationFrame(id);
}
#endif

static void didAddStorageEventListener(DOMWindow* window)
{
    // Creating these WebCore::Storage objects informs the system that we'd like to receive
    // notifications about storage events that might be triggered in other processes. Rather
    // than subscribe to these notifications explicitly, we subscribe to them implicitly to
    // simplify the work done by the system. 
    ExceptionCode unused;
    window->localStorage(unused);
    window->sessionStorage(unused);
}

bool DOMWindow::addEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener, bool useCapture)
{
    if (!EventTarget::addEventListener(eventType, listener, useCapture))
        return false;

    if (Document* document = this->document()) {
        document->addListenerTypeIfNeeded(eventType);
        if (eventType == eventNames().mousewheelEvent)
            document->didAddWheelEventHandler();
        else if (eventNames().isTouchEventType(eventType))
            document->didAddTouchEventHandler();
        else if (eventType == eventNames().storageEvent)
            didAddStorageEventListener(this);
    }

    if (eventType == eventNames().unloadEvent)
        addUnloadEventListener(this);
    else if (eventType == eventNames().beforeunloadEvent && allowsBeforeUnloadListeners(this))
        addBeforeUnloadEventListener(this);
#if ENABLE(DEVICE_ORIENTATION)
    else if (eventType == eventNames().devicemotionEvent && RuntimeEnabledFeatures::deviceMotionEnabled()) {
        if (DeviceMotionController* controller = DeviceMotionController::from(page()))
            controller->addListener(this);
    } else if (eventType == eventNames().deviceorientationEvent && RuntimeEnabledFeatures::deviceOrientationEnabled()) {
        if (DeviceOrientationController* controller = DeviceOrientationController::from(page()))
            controller->addListener(this);
    }
#endif

    return true;
}

bool DOMWindow::removeEventListener(const AtomicString& eventType, EventListener* listener, bool useCapture)
{
    if (!EventTarget::removeEventListener(eventType, listener, useCapture))
        return false;

    if (Document* document = this->document()) {
        if (eventType == eventNames().mousewheelEvent)
            document->didRemoveWheelEventHandler();
        else if (eventNames().isTouchEventType(eventType))
            document->didRemoveTouchEventHandler();
    }

    if (eventType == eventNames().unloadEvent)
        removeUnloadEventListener(this);
    else if (eventType == eventNames().beforeunloadEvent && allowsBeforeUnloadListeners(this))
        removeBeforeUnloadEventListener(this);
#if ENABLE(DEVICE_ORIENTATION)
    else if (eventType == eventNames().devicemotionEvent) {
        if (DeviceMotionController* controller = DeviceMotionController::from(page()))
            controller->removeListener(this);
    } else if (eventType == eventNames().deviceorientationEvent) {
        if (DeviceOrientationController* controller = DeviceOrientationController::from(page()))
            controller->removeListener(this);
    }
#endif

    return true;
}

void DOMWindow::dispatchLoadEvent()
{
    RefPtr<Event> loadEvent(Event::create(eventNames().loadEvent, false, false));
    if (m_frame && m_frame->loader()->documentLoader() && !m_frame->loader()->documentLoader()->timing()->loadEventStart()) {
        // The DocumentLoader (and thus its DocumentLoadTiming) might get destroyed while dispatching
        // the event, so protect it to prevent writing the end time into freed memory.
        RefPtr<DocumentLoader> documentLoader = m_frame->loader()->documentLoader();
        DocumentLoadTiming* timing = documentLoader->timing();
        timing->markLoadEventStart();
        dispatchEvent(loadEvent, document());
        timing->markLoadEventEnd();
    } else
        dispatchEvent(loadEvent, document());

    // For load events, send a separate load event to the enclosing frame only.
    // This is a DOM extension and is independent of bubbling/capturing rules of
    // the DOM.
    Element* ownerElement = m_frame ? m_frame->ownerElement() : 0;
    if (ownerElement)
        ownerElement->dispatchEvent(Event::create(eventNames().loadEvent, false, false));

    InspectorInstrumentation::loadEventFired(frame());
}

bool DOMWindow::dispatchEvent(PassRefPtr<Event> prpEvent, PassRefPtr<EventTarget> prpTarget)
{
    RefPtr<EventTarget> protect = this;
    RefPtr<Event> event = prpEvent;

    event->setTarget(prpTarget ? prpTarget : this);
    event->setCurrentTarget(this);
    event->setEventPhase(Event::AT_TARGET);

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willDispatchEventOnWindow(frame(), *event, this);

    bool result = fireEventListeners(event.get());

    InspectorInstrumentation::didDispatchEventOnWindow(cookie);

    return result;
}

void DOMWindow::removeAllEventListeners()
{
    EventTarget::removeAllEventListeners();

#if ENABLE(DEVICE_ORIENTATION)
    if (DeviceMotionController* controller = DeviceMotionController::from(page()))
        controller->removeAllListeners(this);
    if (DeviceOrientationController* controller = DeviceOrientationController::from(page()))
        controller->removeAllListeners(this);
#endif

    removeAllUnloadEventListeners(this);
    removeAllBeforeUnloadEventListeners(this);
}

void DOMWindow::captureEvents()
{
    // Not implemented.
}

void DOMWindow::releaseEvents()
{
    // Not implemented.
}

void DOMWindow::finishedLoading()
{
    if (m_shouldPrintWhenFinishedLoading) {
        m_shouldPrintWhenFinishedLoading = false;
        print();
    }
}

EventTargetData* DOMWindow::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* DOMWindow::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void DOMWindow::setLocation(const String& urlString, DOMWindow* activeWindow, DOMWindow* firstWindow, SetLocationLocking locking)
{
    if (!isCurrentlyDisplayedInFrame())
        return;

    Document* activeDocument = activeWindow->document();
    if (!activeDocument)
        return;

    if (!activeDocument->canNavigate(m_frame))
        return;

    Frame* firstFrame = firstWindow->frame();
    if (!firstFrame)
        return;

    KURL completedURL = firstFrame->document()->completeURL(urlString);
    if (completedURL.isNull())
        return;

    if (isInsecureScriptAccess(activeWindow, completedURL))
        return;

    // We want a new history item if we are processing a user gesture.
    m_frame->navigationScheduler()->scheduleLocationChange(activeDocument->securityOrigin(),
        // FIXME: What if activeDocument()->frame() is 0?
        completedURL, activeDocument->frame()->loader()->outgoingReferrer(),
        locking != LockHistoryBasedOnGestureState || !ScriptController::processingUserGesture(),
        locking != LockHistoryBasedOnGestureState);
}

void DOMWindow::printErrorMessage(const String& message)
{
    if (message.isEmpty())
        return;

    Settings* settings = m_frame->settings();
    if (!settings)
        return;
    if (settings->privateBrowsingEnabled())
        return;

    // FIXME: Add arguments so that we can provide a correct source URL and line number.
    RefPtr<ScriptCallStack> stackTrace = createScriptCallStack(ScriptCallStack::maxCallStackSizeToCapture, true);
    console()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, stackTrace.release());
}

String DOMWindow::crossDomainAccessErrorMessage(DOMWindow* activeWindow)
{
    const KURL& activeWindowURL = activeWindow->document()->url();
    if (activeWindowURL.isNull())
        return String();

    ASSERT(!activeWindow->document()->securityOrigin()->canAccess(document()->securityOrigin()));

    // FIXME: This message, and other console messages, have extra newlines. Should remove them.
    String message = "Unsafe JavaScript attempt to access frame with URL " + document()->url().string() + " from frame with URL " + activeWindowURL.string() + ".";

    // Sandbox errors.
    if (document()->isSandboxed(SandboxOrigin) || activeWindow->document()->isSandboxed(SandboxOrigin)) {
        if (document()->isSandboxed(SandboxOrigin) && activeWindow->document()->isSandboxed(SandboxOrigin))
            return "Sandbox access violation: " + message + " Both frames are sandboxed into unique origins.\n";
        if (document()->isSandboxed(SandboxOrigin))
            return "Sandbox access violation: " + message + " The frame being accessed is sandboxed into a unique origin.\n";
        return "Sandbox access violation: " + message + " The frame requesting access is sandboxed into a unique origin.\n";
    }

    SecurityOrigin* activeOrigin = activeWindow->document()->securityOrigin();
    SecurityOrigin* targetOrigin = document()->securityOrigin();
    if (targetOrigin->protocol() != activeOrigin->protocol())
        return message + " The frame requesting access has a protocol of '" + activeOrigin->protocol() + "', the frame being accessed has a protocol of '" + targetOrigin->protocol() + "'. Protocols must match.\n";

    // 'document.domain' errors.
    if (targetOrigin->domainWasSetInDOM() && activeOrigin->domainWasSetInDOM())
        return message + " The frame requesting access set 'document.domain' to '" + activeOrigin->domain() + "', the frame being accessed set it to '" + targetOrigin->domain() + "'. Both must set 'document.domain' to the same value to allow access.\n";
    if (activeOrigin->domainWasSetInDOM())
        return message + " The frame requesting access set 'document.domain' to '" + activeOrigin->domain() + "', but the frame being accessed did not. Both must set 'document.domain' to the same value to allow access.\n";
    if (targetOrigin->domainWasSetInDOM())
        return message + " The frame being accessed set 'document.domain' to '" + targetOrigin->domain() + "', but the frame requesting access did not. Both must set 'document.domain' to the same value to allow access.\n";

    // Default.
    return message + " Domains, protocols and ports must match.\n";
}

bool DOMWindow::isInsecureScriptAccess(DOMWindow* activeWindow, const String& urlString)
{
    if (!protocolIsJavaScript(urlString))
        return false;

    // If this DOMWindow isn't currently active in the Frame, then there's no
    // way we should allow the access.
    // FIXME: Remove this check if we're able to disconnect DOMWindow from
    // Frame on navigation: https://bugs.webkit.org/show_bug.cgi?id=62054
    if (isCurrentlyDisplayedInFrame()) {
        // FIXME: Is there some way to eliminate the need for a separate "activeWindow == this" check?
        if (activeWindow == this)
            return false;

        // FIXME: The name canAccess seems to be a roundabout way to ask "can execute script".
        // Can we name the SecurityOrigin function better to make this more clear?
        if (activeWindow->document()->securityOrigin()->canAccess(document()->securityOrigin()))
            return false;
    }

    printErrorMessage(crossDomainAccessErrorMessage(activeWindow));
    return true;
}

Frame* DOMWindow::createWindow(const String& urlString, const AtomicString& frameName, const WindowFeatures& windowFeatures,
    DOMWindow* activeWindow, Frame* firstFrame, Frame* openerFrame, PrepareDialogFunction function, void* functionContext)
{
    Frame* activeFrame = activeWindow->frame();

    // For whatever reason, Firefox uses the first frame to determine the outgoingReferrer. We replicate that behavior here.
    String referrer = firstFrame->loader()->outgoingReferrer();

    KURL completedURL = urlString.isEmpty() ? KURL(ParsedURLString, emptyString()) : firstFrame->document()->completeURL(urlString);
    if (!completedURL.isEmpty() && !completedURL.isValid()) {
        // Don't expose client code to invalid URLs.
        activeWindow->printErrorMessage("Unable to open a window with invalid URL '" + completedURL.string() + "'.\n");
        return 0;
    }

    ResourceRequest request(completedURL, referrer);
    FrameLoader::addHTTPOriginIfNeeded(request, firstFrame->loader()->outgoingOrigin());
    FrameLoadRequest frameRequest(activeWindow->document()->securityOrigin(), request, frameName);

    // We pass the opener frame for the lookupFrame in case the active frame is different from
    // the opener frame, and the name references a frame relative to the opener frame.
    bool created;
    Frame* newFrame = WebCore::createWindow(activeFrame, openerFrame, frameRequest, windowFeatures, created);
    if (!newFrame)
        return 0;

    newFrame->loader()->setOpener(openerFrame);
    newFrame->page()->setOpenedByDOM();

    if (newFrame->document()->domWindow()->isInsecureScriptAccess(activeWindow, completedURL))
        return newFrame;

    if (function)
        function(newFrame->document()->domWindow(), functionContext);

    if (created)
        newFrame->loader()->changeLocation(activeWindow->document()->securityOrigin(), completedURL, referrer, false, false);
    else if (!urlString.isEmpty()) {
        bool lockHistory = !ScriptController::processingUserGesture();
        newFrame->navigationScheduler()->scheduleLocationChange(activeWindow->document()->securityOrigin(), completedURL.string(), referrer, lockHistory, false);
    }

    return newFrame;
}

PassRefPtr<DOMWindow> DOMWindow::open(const String& urlString, const AtomicString& frameName, const String& windowFeaturesString,
    DOMWindow* activeWindow, DOMWindow* firstWindow)
{
    if (!isCurrentlyDisplayedInFrame())
        return 0;
    Document* activeDocument = activeWindow->document();
    if (!activeDocument)
        return 0;
    Frame* firstFrame = firstWindow->frame();
    if (!firstFrame)
        return 0;

    if (!firstWindow->allowPopUp()) {
        // Because FrameTree::find() returns true for empty strings, we must check for empty frame names.
        // Otherwise, illegitimate window.open() calls with no name will pass right through the popup blocker.
        if (frameName.isEmpty() || !m_frame->tree()->find(frameName))
            return 0;
    }

    // Get the target frame for the special cases of _top and _parent.
    // In those cases, we schedule a location change right now and return early.
    Frame* targetFrame = 0;
    if (frameName == "_top")
        targetFrame = m_frame->tree()->top();
    else if (frameName == "_parent") {
        if (Frame* parent = m_frame->tree()->parent())
            targetFrame = parent;
        else
            targetFrame = m_frame;
    }
    if (targetFrame) {
        if (!activeDocument->canNavigate(targetFrame))
            return 0;

        KURL completedURL = firstFrame->document()->completeURL(urlString);

        if (targetFrame->document()->domWindow()->isInsecureScriptAccess(activeWindow, completedURL))
            return targetFrame->document()->domWindow();

        if (urlString.isEmpty())
            return targetFrame->document()->domWindow();

        // For whatever reason, Firefox uses the first window rather than the active window to
        // determine the outgoing referrer. We replicate that behavior here.
        bool lockHistory = !ScriptController::processingUserGesture();
        targetFrame->navigationScheduler()->scheduleLocationChange(
            activeDocument->securityOrigin(),
            completedURL,
            firstFrame->loader()->outgoingReferrer(),
            lockHistory,
            false);
        return targetFrame->document()->domWindow();
    }

    WindowFeatures windowFeatures(windowFeaturesString);
    FloatRect windowRect(windowFeatures.xSet ? windowFeatures.x : 0, windowFeatures.ySet ? windowFeatures.y : 0,
        windowFeatures.widthSet ? windowFeatures.width : 0, windowFeatures.heightSet ? windowFeatures.height : 0);
    Page* page = m_frame->page();
    DOMWindow::adjustWindowRect(screenAvailableRect(page ? page->mainFrame()->view() : 0), windowRect, windowRect);
    windowFeatures.x = windowRect.x();
    windowFeatures.y = windowRect.y();
    windowFeatures.height = windowRect.height();
    windowFeatures.width = windowRect.width();

    Frame* result = createWindow(urlString, frameName, windowFeatures, activeWindow, firstFrame, m_frame);
    return result ? result->document()->domWindow() : 0;
}

void DOMWindow::showModalDialog(const String& urlString, const String& dialogFeaturesString,
    DOMWindow* activeWindow, DOMWindow* firstWindow, PrepareDialogFunction function, void* functionContext)
{
    if (!isCurrentlyDisplayedInFrame())
        return;
    Frame* activeFrame = activeWindow->frame();
    if (!activeFrame)
        return;
    Frame* firstFrame = firstWindow->frame();
    if (!firstFrame)
        return;

    if (!canShowModalDialogNow(m_frame) || !firstWindow->allowPopUp())
        return;

    Frame* dialogFrame = createWindow(urlString, emptyAtom, WindowFeatures(dialogFeaturesString, screenAvailableRect(m_frame->view())),
        activeWindow, firstFrame, m_frame, function, functionContext);
    if (!dialogFrame)
        return;

    dialogFrame->page()->chrome()->runModal();
}

} // namespace WebCore
