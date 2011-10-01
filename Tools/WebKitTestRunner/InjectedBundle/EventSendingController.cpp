/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "EventSendingController.h"

#include "InjectedBundle.h"
#include "InjectedBundlePage.h"
#include "JSEventSendingController.h"
#include "StringFunctions.h"
#include <WebKit2/WKBundle.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKBundlePagePrivate.h>
#include <WebKit2/WKBundlePrivate.h>
#include <WebKit2/WKMutableDictionary.h>
#include <WebKit2/WKNumber.h>

namespace WTR {

static const float ZoomMultiplierRatio = 1.2f;

static WKEventModifiers parseModifier(JSStringRef modifier)
{
    if (JSStringIsEqualToUTF8CString(modifier, "ctrlKey"))
        return kWKEventModifiersControlKey;
    if (JSStringIsEqualToUTF8CString(modifier, "shiftKey") || JSStringIsEqualToUTF8CString(modifier, "rangeSelectionKey"))
        return kWKEventModifiersShiftKey;
    if (JSStringIsEqualToUTF8CString(modifier, "altKey"))
        return kWKEventModifiersAltKey;
    if (JSStringIsEqualToUTF8CString(modifier, "metaKey") || JSStringIsEqualToUTF8CString(modifier, "addSelectionKey"))
        return kWKEventModifiersMetaKey;
    return 0;
}

static unsigned arrayLength(JSContextRef context, JSObjectRef array)
{
    JSRetainPtr<JSStringRef> lengthString(Adopt, JSStringCreateWithUTF8CString("length"));
    JSValueRef lengthValue = JSObjectGetProperty(context, array, lengthString.get(), 0);
    if (!lengthValue)
        return 0;
    return static_cast<unsigned>(JSValueToNumber(context, lengthValue, 0));
}

static WKEventModifiers parseModifierArray(JSContextRef context, JSValueRef arrayValue)
{
    if (!arrayValue)
        return 0;
    if (!JSValueIsObject(context, arrayValue))
        return 0;
    JSObjectRef array = const_cast<JSObjectRef>(arrayValue);
    unsigned length = arrayLength(context, array);
    WKEventModifiers modifiers = 0;
    for (unsigned i = 0; i < length; i++) {
        JSValueRef exception = 0;
        JSValueRef value = JSObjectGetPropertyAtIndex(context, array, i, &exception);
        if (exception)
            continue;
        JSRetainPtr<JSStringRef> string(Adopt, JSValueToStringCopy(context, value, &exception));
        if (exception)
            continue;
        modifiers |= parseModifier(string.get());
    }
    return modifiers;
}

PassRefPtr<EventSendingController> EventSendingController::create()
{
    return adoptRef(new EventSendingController);
}

EventSendingController::EventSendingController()
#ifdef USE_WEBPROCESS_EVENT_SIMULATION
    : m_time(0)
    , m_position()
    , m_clickCount(0)
    , m_clickTime(0)
    , m_clickPosition()
    , m_clickButton(kWKEventMouseButtonNoButton)
#endif
{
}

EventSendingController::~EventSendingController()
{
}

JSClassRef EventSendingController::wrapperClass()
{
    return JSEventSendingController::eventSendingControllerClass();
}

void EventSendingController::mouseDown(int button, JSValueRef modifierArray)
{
    WKBundlePageRef page = InjectedBundle::shared().page()->page();
    WKBundleFrameRef frame = WKBundlePageGetMainFrame(page);
    JSContextRef context = WKBundleFrameGetJavaScriptContext(frame);
    WKEventModifiers modifiers = parseModifierArray(context, modifierArray);

#ifdef USE_WEBPROCESS_EVENT_SIMULATION
    updateClickCount(button);
    WKBundlePageSimulateMouseDown(page, button, m_position, m_clickCount, modifiers, m_time);
#else
    WKRetainPtr<WKStringRef> EventSenderMessageName(AdoptWK, WKStringCreateWithUTF8CString("EventSender"));
    WKRetainPtr<WKMutableDictionaryRef> EventSenderMessageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
    WKRetainPtr<WKStringRef> subMessageName(AdoptWK, WKStringCreateWithUTF8CString("MouseDown"));
    WKDictionaryAddItem(EventSenderMessageBody.get(), subMessageKey.get(), subMessageName.get());

    WKRetainPtr<WKStringRef> buttonKey = adoptWK(WKStringCreateWithUTF8CString("Button"));
    WKRetainPtr<WKUInt64Ref> buttonRef = WKUInt64Create(button);
    WKDictionaryAddItem(EventSenderMessageBody.get(), buttonKey.get(), buttonRef.get());

    WKRetainPtr<WKStringRef> modifiersKey = adoptWK(WKStringCreateWithUTF8CString("Modifiers"));
    WKRetainPtr<WKUInt64Ref> modifiersRef = WKUInt64Create(modifiers);
    WKDictionaryAddItem(EventSenderMessageBody.get(), modifiersKey.get(), modifiersRef.get());

    WKBundlePostSynchronousMessage(InjectedBundle::shared().bundle(), EventSenderMessageName.get(), EventSenderMessageBody.get(), 0);
#endif
}

void EventSendingController::mouseUp(int button, JSValueRef modifierArray)
{
    WKBundlePageRef page = InjectedBundle::shared().page()->page();
    WKBundleFrameRef frame = WKBundlePageGetMainFrame(page);
    JSContextRef context = WKBundleFrameGetJavaScriptContext(frame);
    WKEventModifiers modifiers = parseModifierArray(context, modifierArray);

#ifdef USE_WEBPROCESS_EVENT_SIMULATION
    updateClickCount(button);
    WKBundlePageSimulateMouseUp(page, button, m_position, m_clickCount, modifiers, m_time);
#else
    WKRetainPtr<WKStringRef> EventSenderMessageName(AdoptWK, WKStringCreateWithUTF8CString("EventSender"));
    WKRetainPtr<WKMutableDictionaryRef> EventSenderMessageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
    WKRetainPtr<WKStringRef> subMessageName(AdoptWK, WKStringCreateWithUTF8CString("MouseUp"));
    WKDictionaryAddItem(EventSenderMessageBody.get(), subMessageKey.get(), subMessageName.get());

    WKRetainPtr<WKStringRef> buttonKey = adoptWK(WKStringCreateWithUTF8CString("Button"));
    WKRetainPtr<WKUInt64Ref> buttonRef = WKUInt64Create(button);
    WKDictionaryAddItem(EventSenderMessageBody.get(), buttonKey.get(), buttonRef.get());

    WKRetainPtr<WKStringRef> modifiersKey = adoptWK(WKStringCreateWithUTF8CString("Modifiers"));
    WKRetainPtr<WKUInt64Ref> modifiersRef = WKUInt64Create(modifiers);
    WKDictionaryAddItem(EventSenderMessageBody.get(), modifiersKey.get(), modifiersRef.get());

    WKBundlePostSynchronousMessage(InjectedBundle::shared().bundle(), EventSenderMessageName.get(), EventSenderMessageBody.get(), 0);
#endif
}

void EventSendingController::mouseMoveTo(int x, int y)
{
#ifdef USE_WEBPROCESS_EVENT_SIMULATION
    m_position.x = x;
    m_position.y = y;
    WKBundlePageSimulateMouseMotion(InjectedBundle::shared().page()->page(), m_position, m_time);
#else
    WKRetainPtr<WKStringRef> EventSenderMessageName(AdoptWK, WKStringCreateWithUTF8CString("EventSender"));
    WKRetainPtr<WKMutableDictionaryRef> EventSenderMessageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
    WKRetainPtr<WKStringRef> subMessageName(AdoptWK, WKStringCreateWithUTF8CString("MouseMoveTo"));
    WKDictionaryAddItem(EventSenderMessageBody.get(), subMessageKey.get(), subMessageName.get());

    WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("X"));
    WKRetainPtr<WKDoubleRef> xRef = WKDoubleCreate(x);
    WKDictionaryAddItem(EventSenderMessageBody.get(), xKey.get(), xRef.get());

    WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("Y"));
    WKRetainPtr<WKDoubleRef> yRef = WKDoubleCreate(y);
    WKDictionaryAddItem(EventSenderMessageBody.get(), yKey.get(), yRef.get());

    WKBundlePostSynchronousMessage(InjectedBundle::shared().bundle(), EventSenderMessageName.get(), EventSenderMessageBody.get(), 0);
#endif
}

void EventSendingController::leapForward(int milliseconds)
{
#ifdef USE_WEBPROCESS_EVENT_SIMULATION
    m_time += milliseconds / 1000.0;
#else
    WKRetainPtr<WKStringRef> EventSenderMessageName(AdoptWK, WKStringCreateWithUTF8CString("EventSender"));
    WKRetainPtr<WKMutableDictionaryRef> EventSenderMessageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
    WKRetainPtr<WKStringRef> subMessageName(AdoptWK, WKStringCreateWithUTF8CString("LeapForward"));
    WKDictionaryAddItem(EventSenderMessageBody.get(), subMessageKey.get(), subMessageName.get());

    WKRetainPtr<WKStringRef> timeKey = adoptWK(WKStringCreateWithUTF8CString("TimeInMilliseconds"));
    WKRetainPtr<WKUInt64Ref> timeRef = WKUInt64Create(milliseconds);
    WKDictionaryAddItem(EventSenderMessageBody.get(), timeKey.get(), timeRef.get());

    WKBundlePostSynchronousMessage(InjectedBundle::shared().bundle(), EventSenderMessageName.get(), EventSenderMessageBody.get(), 0);
#endif
}

void EventSendingController::keyDown(JSStringRef key, JSValueRef modifierArray, int location)
{
    WKBundlePageRef page = InjectedBundle::shared().page()->page();
    WKBundleFrameRef frame = WKBundlePageGetMainFrame(page);
    JSContextRef context = WKBundleFrameGetJavaScriptContext(frame);
    WKEventModifiers modifiers = parseModifierArray(context, modifierArray);

    WKRetainPtr<WKStringRef> EventSenderMessageName(AdoptWK, WKStringCreateWithUTF8CString("EventSender"));
    WKRetainPtr<WKMutableDictionaryRef> EventSenderMessageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
    WKRetainPtr<WKStringRef> subMessageName(AdoptWK, WKStringCreateWithUTF8CString("KeyDown"));
    WKDictionaryAddItem(EventSenderMessageBody.get(), subMessageKey.get(), subMessageName.get());

    WKRetainPtr<WKStringRef> keyKey = adoptWK(WKStringCreateWithUTF8CString("Key"));
    WKDictionaryAddItem(EventSenderMessageBody.get(), keyKey.get(), toWK(key).get());

    WKRetainPtr<WKStringRef> modifiersKey = adoptWK(WKStringCreateWithUTF8CString("Modifiers"));
    WKRetainPtr<WKUInt64Ref> modifiersRef = WKUInt64Create(modifiers);
    WKDictionaryAddItem(EventSenderMessageBody.get(), modifiersKey.get(), modifiersRef.get());

    WKRetainPtr<WKStringRef> locationKey = adoptWK(WKStringCreateWithUTF8CString("Location"));
    WKRetainPtr<WKUInt64Ref> locationRef = WKUInt64Create(location);
    WKDictionaryAddItem(EventSenderMessageBody.get(), locationKey.get(), locationRef.get());

    WKBundlePostSynchronousMessage(InjectedBundle::shared().bundle(), EventSenderMessageName.get(), EventSenderMessageBody.get(), 0);
}

void EventSendingController::mouseScrollBy(int x, int y)
{
    WKRetainPtr<WKStringRef> EventSenderMessageName(AdoptWK, WKStringCreateWithUTF8CString("EventSender"));
    WKRetainPtr<WKMutableDictionaryRef> EventSenderMessageBody(AdoptWK, WKMutableDictionaryCreate());

    WKRetainPtr<WKStringRef> subMessageKey(AdoptWK, WKStringCreateWithUTF8CString("SubMessage"));
    WKRetainPtr<WKStringRef> subMessageName(AdoptWK, WKStringCreateWithUTF8CString("MouseScrollBy"));
    WKDictionaryAddItem(EventSenderMessageBody.get(), subMessageKey.get(), subMessageName.get());

    WKRetainPtr<WKStringRef> xKey = adoptWK(WKStringCreateWithUTF8CString("X"));
    WKRetainPtr<WKDoubleRef> xRef = WKDoubleCreate(x);
    WKDictionaryAddItem(EventSenderMessageBody.get(), xKey.get(), xRef.get());

    WKRetainPtr<WKStringRef> yKey = adoptWK(WKStringCreateWithUTF8CString("Y"));
    WKRetainPtr<WKDoubleRef> yRef = WKDoubleCreate(y);
    WKDictionaryAddItem(EventSenderMessageBody.get(), yKey.get(), yRef.get());

    WKBundlePostSynchronousMessage(InjectedBundle::shared().bundle(), EventSenderMessageName.get(), EventSenderMessageBody.get(), 0);
}

#ifdef USE_WEBPROCESS_EVENT_SIMULATION
void EventSendingController::updateClickCount(WKEventMouseButton button)
{
    if (m_time - m_clickTime < 1 && m_position == m_clickPosition && button == m_clickButton) {
        ++m_clickCount;
        m_clickTime = m_time;
        return;
    }

    m_clickCount = 1;
    m_clickTime = m_time;
    m_clickPosition = m_position;
    m_clickButton = button;
}
#endif

void EventSendingController::textZoomIn()
{
    // Ensure page zoom is reset.
    WKBundlePageSetPageZoomFactor(InjectedBundle::shared().page()->page(), 1);

    double zoomFactor = WKBundlePageGetTextZoomFactor(InjectedBundle::shared().page()->page());
    WKBundlePageSetTextZoomFactor(InjectedBundle::shared().page()->page(), zoomFactor * ZoomMultiplierRatio);
}

void EventSendingController::textZoomOut()
{
    // Ensure page zoom is reset.
    WKBundlePageSetPageZoomFactor(InjectedBundle::shared().page()->page(), 1);

    double zoomFactor = WKBundlePageGetTextZoomFactor(InjectedBundle::shared().page()->page());
    WKBundlePageSetTextZoomFactor(InjectedBundle::shared().page()->page(), zoomFactor / ZoomMultiplierRatio);
}

void EventSendingController::zoomPageIn()
{
    // Ensure text zoom is reset.
    WKBundlePageSetTextZoomFactor(InjectedBundle::shared().page()->page(), 1);

    double zoomFactor = WKBundlePageGetPageZoomFactor(InjectedBundle::shared().page()->page());
    WKBundlePageSetPageZoomFactor(InjectedBundle::shared().page()->page(), zoomFactor * ZoomMultiplierRatio);
}

void EventSendingController::zoomPageOut()
{
    // Ensure text zoom is reset.
    WKBundlePageSetTextZoomFactor(InjectedBundle::shared().page()->page(), 1);

    double zoomFactor = WKBundlePageGetPageZoomFactor(InjectedBundle::shared().page()->page());
    WKBundlePageSetPageZoomFactor(InjectedBundle::shared().page()->page(), zoomFactor / ZoomMultiplierRatio);
}

void EventSendingController::scalePageBy(double scale, double x, double y)
{
    WKPoint origin = { x, y };
    WKBundlePageSetScaleAtOrigin(InjectedBundle::shared().page()->page(), scale, origin);
}

// Object Creation

void EventSendingController::makeWindowObject(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception)
{
    setProperty(context, windowObject, "eventSender", this, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, exception);
}

} // namespace WTR
