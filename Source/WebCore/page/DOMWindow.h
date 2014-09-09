/*
 * Copyright (C) 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DOMWindow_h
#define DOMWindow_h

#include "ContextDestructionObserver.h"
#include "EventTarget.h"
#include "FrameDestructionObserver.h"
#include "URL.h"
#include "Supplementable.h"
#include <functional>
#include <memory>

namespace Inspector {
class ScriptCallStack;
}

namespace WebCore {

    class BarProp;
    class CSSRuleList;
    class CSSStyleDeclaration;
    class Crypto;
    class DOMApplicationCache;
    class DOMSelection;
    class DOMURL;
    class DOMWindowCSS;
    class DOMWindowProperty;
    class DOMWrapperWorld;
    class Database;
    class DatabaseCallback;
    class Document;
    class Element;
    class EventListener;
    class FloatRect;
    class Frame;
    class History;
    class IDBFactory;
    class Location;
    class MediaQueryList;
    class MessageEvent;
    class Navigator;
    class Node;
    class Page;
    class PageConsole;
    class Performance;
    class PostMessageTimer;
    class ScheduledAction;
    class Screen;
    class SecurityOrigin;
    class SerializedScriptValue;
    class Storage;
    class StyleMedia;
    class WebKitNamespace;
    class WebKitPoint;

#if ENABLE(REQUEST_ANIMATION_FRAME)
    class RequestAnimationFrameCallback;
#endif

    struct WindowFeatures;

    typedef Vector<RefPtr<MessagePort>, 1> MessagePortArray;

    typedef int ExceptionCode;

    enum SetLocationLocking { LockHistoryBasedOnGestureState, LockHistoryAndBackForwardList };

    // FIXME: DOMWindow shouldn't subclass FrameDestructionObserver and instead should get to Frame via its Document.
    class DOMWindow final
        : public RefCounted<DOMWindow>
        , public EventTargetWithInlineData
        , public ContextDestructionObserver
        , public FrameDestructionObserver
        , public Supplementable<DOMWindow> {
    public:
        static PassRefPtr<DOMWindow> create(Document* document) { return adoptRef(new DOMWindow(document)); }
        virtual ~DOMWindow();

        // In some rare cases, we'll re-used a DOMWindow for a new Document. For example,
        // when a script calls window.open("..."), the browser gives JavaScript a window
        // synchronously but kicks off the load in the window asynchronously. Web sites
        // expect that modifications that they make to the window object synchronously
        // won't be blown away when the network load commits. To make that happen, we
        // "securely transition" the existing DOMWindow to the Document that results from
        // the network load. See also SecurityContext::isSecureTransitionTo.
        void didSecureTransitionTo(Document*);

        virtual EventTargetInterface eventTargetInterface() const override { return DOMWindowEventTargetInterfaceType; }
        virtual ScriptExecutionContext* scriptExecutionContext() const override { return ContextDestructionObserver::scriptExecutionContext(); }

        virtual DOMWindow* toDOMWindow() override;

        void registerProperty(DOMWindowProperty*);
        void unregisterProperty(DOMWindowProperty*);

        void resetUnlessSuspendedForPageCache();
        void suspendForPageCache();
        void resumeFromPageCache();

        PassRefPtr<MediaQueryList> matchMedia(const String&);

        unsigned pendingUnloadEventListeners() const;

        static bool dispatchAllPendingBeforeUnloadEvents();
        static void dispatchAllPendingUnloadEvents();

        static FloatRect adjustWindowRect(Page*, const FloatRect& pendingChanges);

        bool allowPopUp(); // Call on first window, not target window.
        static bool allowPopUp(Frame* firstFrame);
        static bool canShowModalDialog(const Frame*);
        static bool canShowModalDialogNow(const Frame*);

        // DOM Level 0

        Screen* screen() const;
        History* history() const;
        Crypto* crypto() const;
        BarProp* locationbar() const;
        BarProp* menubar() const;
        BarProp* personalbar() const;
        BarProp* scrollbars() const;
        BarProp* statusbar() const;
        BarProp* toolbar() const;
        Navigator* navigator() const;
        Navigator* clientInformation() const { return navigator(); }

        Location* location() const;
        void setLocation(const String& location, DOMWindow& activeWindow, DOMWindow& firstWindow,
            SetLocationLocking = LockHistoryBasedOnGestureState);

        DOMSelection* getSelection();

        Element* frameElement() const;

        void focus(ScriptExecutionContext* = 0);
        void blur();
        void close(ScriptExecutionContext* = 0);
        void print();
        void stop();

        PassRefPtr<DOMWindow> open(const String& urlString, const AtomicString& frameName, const String& windowFeaturesString,
            DOMWindow& activeWindow, DOMWindow& firstWindow);

        void showModalDialog(const String& urlString, const String& dialogFeaturesString, DOMWindow& activeWindow, DOMWindow& firstWindow, std::function<void (DOMWindow&)> prepareDialogFunction);

        void alert(const String& message);
        bool confirm(const String& message);
        String prompt(const String& message, const String& defaultValue);
        String btoa(const String& stringToEncode, ExceptionCode&);
        String atob(const String& encodedString, ExceptionCode&);

        bool find(const String&, bool caseSensitive, bool backwards, bool wrap, bool wholeWord, bool searchInFrames, bool showDialog) const;

        bool offscreenBuffering() const;

        int outerHeight() const;
        int outerWidth() const;
        int innerHeight() const;
        int innerWidth() const;
        int screenX() const;
        int screenY() const;
        int screenLeft() const { return screenX(); }
        int screenTop() const { return screenY(); }
        int scrollX() const;
        int scrollY() const;
        int pageXOffset() const { return scrollX(); }
        int pageYOffset() const { return scrollY(); }

        bool closed() const;

        unsigned length() const;

        String name() const;
        void setName(const String&);

        String status() const;
        void setStatus(const String&);
        String defaultStatus() const;
        void setDefaultStatus(const String&);

        // Self-referential attributes

        DOMWindow* self() const;
        DOMWindow* window() const { return self(); }
        DOMWindow* frames() const { return self(); }

        DOMWindow* opener() const;
        DOMWindow* parent() const;
        DOMWindow* top() const;

        // DOM Level 2 AbstractView Interface

        Document* document() const;

        // CSSOM View Module

        PassRefPtr<StyleMedia> styleMedia() const;

        // DOM Level 2 Style Interface

        PassRefPtr<CSSStyleDeclaration> getComputedStyle(Element*, const String& pseudoElt) const;

        // WebKit extensions

        PassRefPtr<CSSRuleList> getMatchedCSSRules(Element*, const String& pseudoElt, bool authorOnly = true) const;
        double devicePixelRatio() const;

        PassRefPtr<WebKitPoint> webkitConvertPointFromPageToNode(Node*, const WebKitPoint*) const;
        PassRefPtr<WebKitPoint> webkitConvertPointFromNodeToPage(Node*, const WebKitPoint*) const;

        PageConsole* pageConsole() const;

        void printErrorMessage(const String&);
        String crossDomainAccessErrorMessage(const DOMWindow& activeWindow);

        void postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray*, const String& targetOrigin, DOMWindow& source, ExceptionCode&);
        // Needed for Objective-C bindings (see bug 28774).
        void postMessage(PassRefPtr<SerializedScriptValue> message, MessagePort*, const String& targetOrigin, DOMWindow& source, ExceptionCode&);
        void postMessageTimerFired(PostMessageTimer&);
        void dispatchMessageEventWithOriginCheck(SecurityOrigin* intendedTargetOrigin, PassRefPtr<Event>, PassRefPtr<Inspector::ScriptCallStack>);

        void scrollBy(int x, int y) const;
        void scrollTo(int x, int y) const;
        void scroll(int x, int y) const { scrollTo(x, y); }

        void moveBy(float x, float y) const;
        void moveTo(float x, float y) const;

        void resizeBy(float x, float y) const;
        void resizeTo(float width, float height) const;

        // Timers
        int setTimeout(std::unique_ptr<ScheduledAction>, int timeout, ExceptionCode&);
        void clearTimeout(int timeoutId);
        int setInterval(std::unique_ptr<ScheduledAction>, int timeout, ExceptionCode&);
        void clearInterval(int timeoutId);

        // WebKit animation extensions
#if ENABLE(REQUEST_ANIMATION_FRAME)
        int requestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback>);
        int webkitRequestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback>);
        void cancelAnimationFrame(int id);
#endif

#if ENABLE(CSS3_CONDITIONAL_RULES)
        DOMWindowCSS* css();
#endif

        // Events
        // EventTarget API
        virtual bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture) override;
        virtual bool removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture) override;
        virtual void removeAllEventListeners() override;

        using EventTarget::dispatchEvent;
        bool dispatchEvent(PassRefPtr<Event> prpEvent, PassRefPtr<EventTarget> prpTarget);

        void dispatchLoadEvent();

        DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(beforeunload);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(blur);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(canplay);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(canplaythrough);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(change);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(click);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(contextmenu);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dblclick);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(drag);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragenter);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragleave);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragover);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(drop);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(durationchange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(emptied);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(ended);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(focus);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(hashchange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(input);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(invalid);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(keydown);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(keypress);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(keyup);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(load);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(loadeddata);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(loadedmetadata);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(loadstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(message);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mousedown);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseenter);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseleave);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mousemove);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseout);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseover);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseup);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mousewheel);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(offline);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(online);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(pagehide);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(pageshow);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(pause);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(play);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(playing);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(popstate);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(ratechange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(reset);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(resize);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(scroll);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(search);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(seeked);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(seeking);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(select);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(stalled);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(storage);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(submit);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(suspend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(timeupdate);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(unload);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(volumechange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(waiting);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitbeginfullscreen);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitendfullscreen);
#if ENABLE(WILL_REVEAL_EDGE_EVENTS)
        DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitwillrevealbottom);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitwillrevealleft);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitwillrevealright);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitwillrevealtop);
#endif
        DEFINE_ATTRIBUTE_EVENT_LISTENER(wheel);

        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkitanimationstart, webkitAnimationStart);
        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkitanimationiteration, webkitAnimationIteration);
        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkitanimationend, webkitAnimationEnd);
        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkittransitionend, webkitTransitionEnd);
        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(transitionend, transitionend);

        void captureEvents();
        void releaseEvents();

        void finishedLoading();

        using RefCounted<DOMWindow>::ref;
        using RefCounted<DOMWindow>::deref;

#if ENABLE(DEVICE_ORIENTATION)
        DEFINE_ATTRIBUTE_EVENT_LISTENER(devicemotion);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(deviceorientation);
#endif

#if ENABLE(PROXIMITY_EVENTS)
        DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitdeviceproximity);
#endif

        // HTML 5 key/value storage
        Storage* sessionStorage(ExceptionCode&) const;
        Storage* localStorage(ExceptionCode&) const;
        Storage* optionalSessionStorage() const { return m_sessionStorage.get(); }
        Storage* optionalLocalStorage() const { return m_localStorage.get(); }

        DOMApplicationCache* applicationCache() const;
        DOMApplicationCache* optionalApplicationCache() const { return m_applicationCache.get(); }

#if ENABLE(ORIENTATION_EVENTS)
        // This is the interface orientation in degrees. Some examples are:
        //  0 is straight up; -90 is when the device is rotated 90 clockwise;
        //  90 is when rotated counter clockwise.
        int orientation() const;

        DEFINE_ATTRIBUTE_EVENT_LISTENER(orientationchange);
#endif

#if ENABLE(TOUCH_EVENTS)
        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchmove);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchcancel);
#endif

#if ENABLE(IOS_GESTURE_EVENTS)
        DEFINE_ATTRIBUTE_EVENT_LISTENER(gesturestart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(gesturechange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(gestureend);
#endif

#if ENABLE(WEB_TIMING)
        Performance* performance() const;
#endif

#if PLATFORM(IOS)
        void incrementScrollEventListenersCount();
        void decrementScrollEventListenersCount();
        unsigned scrollEventListenerCount() const { return m_scrollEventListenerCount; }
#endif

        void resetAllGeolocationPermission();

#if ENABLE(IOS_TOUCH_EVENTS) || ENABLE(IOS_GESTURE_EVENTS)
        bool hasTouchEventListeners() const { return m_touchEventListenerCount > 0; }
#endif

#if ENABLE(USER_MESSAGE_HANDLERS)
        bool shouldHaveWebKitNamespaceForWorld(DOMWrapperWorld&);
        WebKitNamespace* webkitNamespace() const;
#endif

        // FIXME: When this DOMWindow is no longer the active DOMWindow (i.e.,
        // when its document is no longer the document that is displayed in its
        // frame), we would like to zero out m_frame to avoid being confused
        // by the document that is currently active in m_frame.
        bool isCurrentlyDisplayedInFrame() const;

        void willDetachDocumentFromFrame();
        void willDestroyCachedFrame();

        void enableSuddenTermination();
        void disableSuddenTermination();

    private:
        explicit DOMWindow(Document*);

        Page* page();
        bool allowedToChangeWindowGeometry() const;

        virtual void frameDestroyed() override;
        virtual void willDetachPage() override;

        virtual void refEventTarget() override { ref(); }
        virtual void derefEventTarget() override { deref(); }

        static PassRefPtr<Frame> createWindow(const String& urlString, const AtomicString& frameName, const WindowFeatures&, DOMWindow& activeWindow, Frame* firstFrame, Frame* openerFrame, std::function<void (DOMWindow&)> prepareDialogFunction = nullptr);
        bool isInsecureScriptAccess(DOMWindow& activeWindow, const String& urlString);

        void resetDOMWindowProperties();
        void disconnectDOMWindowProperties();
        void reconnectDOMWindowProperties();
        void willDestroyDocumentInFrame();

#if ENABLE(GAMEPAD)
        void incrementGamepadEventListenerCount();
        void decrementGamepadEventListenerCount();
#endif

        bool m_shouldPrintWhenFinishedLoading;
        bool m_suspendedForPageCache;

        HashSet<DOMWindowProperty*> m_properties;

        mutable RefPtr<Screen> m_screen;
        mutable RefPtr<History> m_history;
        mutable RefPtr<Crypto>  m_crypto;
        mutable RefPtr<BarProp> m_locationbar;
        mutable RefPtr<BarProp> m_menubar;
        mutable RefPtr<BarProp> m_personalbar;
        mutable RefPtr<BarProp> m_scrollbars;
        mutable RefPtr<BarProp> m_statusbar;
        mutable RefPtr<BarProp> m_toolbar;
        mutable RefPtr<Navigator> m_navigator;
        mutable RefPtr<Location> m_location;
        mutable RefPtr<StyleMedia> m_media;

        String m_status;
        String m_defaultStatus;

        enum PageStatus { PageStatusNone, PageStatusShown, PageStatusHidden };
        PageStatus m_lastPageStatus;

#if PLATFORM(IOS)
        unsigned m_scrollEventListenerCount;
#endif

#if ENABLE(IOS_TOUCH_EVENTS) || ENABLE(IOS_GESTURE_EVENTS)
        unsigned m_touchEventListenerCount;
#endif

#if ENABLE(GAMEPAD)
        unsigned m_gamepadEventListenerCount;
#endif
        mutable RefPtr<Storage> m_sessionStorage;
        mutable RefPtr<Storage> m_localStorage;
        mutable RefPtr<DOMApplicationCache> m_applicationCache;

#if ENABLE(WEB_TIMING)
        mutable RefPtr<Performance> m_performance;
#endif

#if ENABLE(CSS3_CONDITIONAL_RULES)
        mutable RefPtr<DOMWindowCSS> m_css;
#endif

#if ENABLE(USER_MESSAGE_HANDLERS)
        mutable RefPtr<WebKitNamespace> m_webkitNamespace;
#endif
    };

    inline String DOMWindow::status() const
    {
        return m_status;
    }

    inline String DOMWindow::defaultStatus() const
    {
        return m_defaultStatus;
    }

} // namespace WebCore

#endif // DOMWindow_h
