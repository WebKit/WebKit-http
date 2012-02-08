/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2009, 2010, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "EventSender.h"

#include "DumpRenderTreeBlackBerry.h"
#include "DumpRenderTreeSupport.h"
#include "IntPoint.h"
#include "NotImplemented.h"
#include "WebPage.h"

#include <BlackBerryPlatformKeyboardEvent.h>
#include <BlackBerryPlatformMouseEvent.h>
#include <BlackBerryPlatformTouchEvent.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSValueRef.h>
#include <wtf/Vector.h>

using namespace WebCore;

static IntPoint lastMousePosition;
static Vector<BlackBerry::Platform::TouchPoint> touches;
static bool touchActive = false;

void sendTouchEvent(BlackBerry::Platform::TouchEvent::Type);

// Callbacks

static JSValueRef getDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static bool setDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseWheelToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*    exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef contextClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef*    exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    page->mouseEvent(BlackBerry::Platform::MouseEvent(BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, 0, lastMousePosition, IntPoint::zero(), 0, 0));
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseUpCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    page->mouseEvent(BlackBerry::Platform::MouseEvent(0, BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, lastMousePosition, IntPoint::zero(), 0, 0));
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseMoveToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int x = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);

    lastMousePosition = IntPoint(x, y);
    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    page->mouseEvent(BlackBerry::Platform::MouseEvent(BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, BlackBerry::Platform::MouseEvent::ScreenLeftMouseButton, lastMousePosition, IntPoint::zero(), 0, 0));


    return JSValueMakeUndefined(context);
}

static JSValueRef beginDragWithFilesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef leapForwardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    JSStringRef character = JSValueToStringCopy(context, arguments[0], exception);
    ASSERT(!*exception);
    short charCode = 0;
    bool needsShiftKeyModifier = false;
    if (JSStringIsEqualToUTF8CString(character, "leftArrow"))
        charCode = KEYCODE_LEFT;
    else if (JSStringIsEqualToUTF8CString(character, "rightArrow"))
        charCode = KEYCODE_RIGHT;
    else if (JSStringIsEqualToUTF8CString(character, "upArrow"))
        charCode = KEYCODE_UP;
    else if (JSStringIsEqualToUTF8CString(character, "downArrow"))
        charCode = KEYCODE_DOWN;
    else if (JSStringIsEqualToUTF8CString(character, "pageUp")
             || JSStringIsEqualToUTF8CString(character, "pageDown")
             || JSStringIsEqualToUTF8CString(character, "home")
             || JSStringIsEqualToUTF8CString(character, "end"))
         return JSValueMakeUndefined(context);
    else if (JSStringIsEqualToUTF8CString(character, "delete"))
        charCode = KEYCODE_BACKSPACE;
    else {
        charCode = JSStringGetCharactersPtr(character)[0];
        if (WTF::isASCIIUpper(charCode))
            needsShiftKeyModifier = true;
    }
    JSStringRelease(character);

    static const JSStringRef lengthProperty = JSStringCreateWithUTF8CString("length");
    bool needsAltKeyModifier = false;
    if (argumentCount > 1) {
        if (JSObjectRef modifiersArray = JSValueToObject(context, arguments[1], 0)) {
            int modifiersCount = JSValueToNumber(context, JSObjectGetProperty(context, modifiersArray, lengthProperty, 0), 0);
            for (int i = 0; i < modifiersCount; ++i) {
                JSStringRef string = JSValueToStringCopy(context, JSObjectGetPropertyAtIndex(context, modifiersArray, i, 0), 0);
                if (JSStringIsEqualToUTF8CString(string, "shiftKey"))
                    needsShiftKeyModifier = true;
                else if (JSStringIsEqualToUTF8CString(string, "altKey"))
                    needsAltKeyModifier = true;
                JSStringRelease(string);
            }
        }
    }

    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();

    unsigned modifiers = 0;
    if (needsShiftKeyModifier)
        modifiers |= KEYMOD_SHIFT;
    if (needsAltKeyModifier)
        modifiers |= KEYMOD_ALT;

    page->keyEvent(BlackBerry::Platform::KeyboardEvent(charCode, BlackBerry::Platform::KeyboardEvent::KeyChar, modifiers));

    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef addTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int x = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);

    BlackBerry::Platform::TouchPoint touch;
    touch.m_id = touches.isEmpty() ? 0 : touches.last().m_id + 1;
    IntPoint pos(x, y);
    touch.m_pos = pos;
    touch.m_screenPos = pos;
    touch.m_state = BlackBerry::Platform::TouchPoint::TouchPressed;

    touches.append(touch);

    return JSValueMakeUndefined(context);
}

static JSValueRef updateTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    int index = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int x = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[2], exception));
    ASSERT(!exception || !*exception);

    if (index < 0 || index >= (int)touches.size())
        return JSValueMakeUndefined(context);

    BlackBerry::Platform::TouchPoint& touch = touches[index];
    IntPoint pos(x, y);
    touch.m_pos = pos;
    touch.m_screenPos = pos;
    touch.m_state = BlackBerry::Platform::TouchPoint::TouchMoved;

    return JSValueMakeUndefined(context);
}

static JSValueRef setTouchModifierCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef touchStartCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!touchActive) {
        sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchStart);
        touchActive = true;
    } else
        sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchMove);
    return JSValueMakeUndefined(context);
}

static JSValueRef touchCancelCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef touchMoveCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchMove);
    return JSValueMakeUndefined(context);
}

static JSValueRef touchEndCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    for (unsigned i = 0; i < touches.size(); ++i)
        if (touches[i].m_state != BlackBerry::Platform::TouchPoint::TouchReleased) {
            sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchMove);
            return JSValueMakeUndefined(context);
        }
    sendTouchEvent(BlackBerry::Platform::TouchEvent::TouchEnd);
    touchActive = false;
    return JSValueMakeUndefined(context);
}

static JSValueRef clearTouchPointsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    touches.clear();
    touchActive = false;
    return JSValueMakeUndefined(context);
}

static JSValueRef cancelTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef releaseTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    int index = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    if (index < 0 || index >= (int)touches.size())
        return JSValueMakeUndefined(context);

    touches[index].m_state = BlackBerry::Platform::TouchPoint::TouchReleased;
    return JSValueMakeUndefined(context);
}

void sendTouchEvent(BlackBerry::Platform::TouchEvent::Type type)
{
    BlackBerry::Platform::TouchEvent event;
    event.m_type = type;
    event.m_points.assign(touches.begin(), touches.end());
    BlackBerry::WebKit::DumpRenderTree::currentInstance()->page()->touchEvent(event);

    Vector<BlackBerry::Platform::TouchPoint> t;

    for (Vector<BlackBerry::Platform::TouchPoint>::iterator it = touches.begin(); it != touches.end(); ++it) {
        if (it->m_state != BlackBerry::Platform::TouchPoint::TouchReleased) {
            it->m_state = BlackBerry::Platform::TouchPoint::TouchStationary;
            t.append(*it);
        }
    }
    touches = t;
}

static JSValueRef scalePageByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    float scaleFactor = JSValueToNumber(context, arguments[0], exception);
    float x = JSValueToNumber(context, arguments[1], exception);
    float y = JSValueToNumber(context, arguments[2], exception);

    BlackBerry::WebKit::WebPage* page = BlackBerry::WebKit::DumpRenderTree::currentInstance()->page();
    if (!page)
        return JSValueMakeUndefined(context);

    DumpRenderTreeSupport::scalePageBy(page, scaleFactor, x, y);

    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "mouseWheelTo", mouseWheelToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "contextClick", contextClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "beginDragWithFiles", beginDragWithFilesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "leapForward", leapForwardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomIn", textZoomInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomOut", textZoomOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "addTouchPoint", addTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "cancelTouchPoint", cancelTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "clearTouchPoints", clearTouchPointsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "releaseTouchPoint", releaseTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scalePageBy", scalePageByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "setTouchModifier", setTouchModifierCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchCancel", touchCancelCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchEnd", touchEndCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchMove", touchMoveCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchStart", touchStartCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "updateTouchPoint", updateTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageIn", zoomPageInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageOut", zoomPageOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSStaticValue staticValues[] = {
    { "dragMode", getDragModeCallback, setDragModeCallback, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSClassRef getClass(JSContextRef context)
{
    static JSClassRef eventSenderClass = 0;

    if (!eventSenderClass) {
        JSClassDefinition classDefinition = {
                0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        classDefinition.staticFunctions = staticFunctions;
        classDefinition.staticValues = staticValues;

        eventSenderClass = JSClassCreate(&classDefinition);
    }

    return eventSenderClass;
}

void replaySavedEvents()
{
    notImplemented();
}

JSObjectRef makeEventSender(JSContextRef context)
{
    return JSObjectMake(context, getClass(context), 0);
}

