/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011, 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EventSender.h"

#include "DumpRenderTree.h"
#include "IntPoint.h"
#include "JSStringUtils.h"
#include "NotImplemented.h"
#include "PlatformEvent.h"

#include <DumpRenderTreeClient.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <WebPage.h>
#include <WebView.h>
#include <wtf/ASCIICType.h>
#include <wtf/Platform.h>
#include <wtf/text/CString.h>

#include <InterfaceDefs.h>
#include <View.h>
#include <Window.h>

static bool gDragMode;
static int gTimeOffset = 0;

static int gLastMousePositionX;
static int gLastMousePositionY;
static int gLastClickPositionX;
static int gLastClickPositionY;
static int gLastClickTimeOffset;
static int gLastClickButton;
static int gButtonCurrentlyDown;
static int gClickCount;

static const float zoomMultiplierRatio = 1.2f;

// Key event location code defined in DOM Level 3.
enum KeyLocationCode {
    DomKeyLocationStandard,
    DomKeyLocationLeft,
    DomKeyLocationRight,
    DomKeyLocationNumpad
};

enum ZoomEvent {
    ZoomIn,
    ZoomOut
};

enum EventQueueStrategy {
    FeedQueuedEvents,
    DoNotFeedQueuedEvents
};

struct KeyEventInfo {
    KeyEventInfo(const CString& keyName, unsigned modifiers, const CString& keyString = CString())
        : keyName(keyName)
        , keyString(keyString)
        , modifiers(modifiers)
    {
    }

    const CString keyName;
    const CString keyString;
    unsigned modifiers;
};

static unsigned touchModifiers;

WTF::Vector<BMessage*>& delayedEventQueue()
{
    DEFINE_STATIC_LOCAL(WTF::Vector<BMessage*>, staticDelayedEventQueue, ());
    return staticDelayedEventQueue;
}


static void feedOrQueueMouseEvent(BMessage*, EventQueueStrategy);
static void feedQueuedMouseEvents();

static int32 translateMouseButtonNumber(int eventSenderButtonNumber)
{
    static const int32 translationTable[] = {
        B_PRIMARY_MOUSE_BUTTON,
        B_TERTIARY_MOUSE_BUTTON,
        B_SECONDARY_MOUSE_BUTTON,
        B_TERTIARY_MOUSE_BUTTON // fast/events/mouse-click-events expects the 4th button to be treated as the middle button
    };
    static const unsigned translationTableSize = sizeof(translationTable) / sizeof(translationTable[0]);

    if (eventSenderButtonNumber < translationTableSize)
        return translationTable[eventSenderButtonNumber];

    return 0;
}

static JSValueRef scheduleAsynchronousClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static void updateClickCount(int button)
{
    if (gLastClickPositionX != gLastMousePositionX
        || gLastClickPositionY != gLastMousePositionY
        || gLastClickButton != button
        || gTimeOffset - gLastClickTimeOffset >= 1)
        gClickCount = 1;
    else
        gClickCount++;
}

static int32 modifierFromJSValue(JSContextRef context, const JSValueRef value)
{
    JSRetainPtr<JSStringRef> jsKeyValue(Adopt, JSValueToStringCopy(context, value, 0));

    if (equals(jsKeyValue, "ctrlKey") || equals(jsKeyValue, "addSelectionKey"))
        return B_CONTROL_KEY;
    if (equals(jsKeyValue, "shiftKey") || equals(jsKeyValue, "rangeSelectionKey"))
        return B_SHIFT_KEY;
    if (equals(jsKeyValue, "altKey"))
        return B_COMMAND_KEY;
    if (equals(jsKeyValue, "metaKey"))
        return B_OPTION_KEY;

    return 0;
}

static unsigned modifiersFromJSValue(JSContextRef context, const JSValueRef modifiers)
{
    // The value may either be a string with a single modifier or an array of modifiers.
    if (JSValueIsString(context, modifiers))
        return modifierFromJSValue(context, modifiers);

    JSObjectRef modifiersArray = JSValueToObject(context, modifiers, 0);
    if (!modifiersArray)
        return 0;

    unsigned modifier = 0;
    JSRetainPtr<JSStringRef> lengthProperty(Adopt, JSStringCreateWithUTF8CString("length"));
    int modifiersCount = JSValueToNumber(context, JSObjectGetProperty(context, modifiersArray, lengthProperty.get(), 0), 0);
    for (int i = 0; i < modifiersCount; ++i)
        modifier |= modifierFromJSValue(context, JSObjectGetPropertyAtIndex(context, modifiersArray, i, 0));
    return modifier;
}

static JSValueRef getMenuItemTitleCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeString(context, JSStringCreateWithUTF8CString(""));
}

static bool setMenuItemTitleCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    return true;
}

static JSValueRef menuItemClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticMenuItemFunctions[] = {
    { "click", menuItemClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSStaticValue staticMenuItemValues[] = {
    { "title", getMenuItemTitleCallback, setMenuItemTitleCallback, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSClassRef getMenuItemClass()
{
    static JSClassRef menuItemClass = 0;

    if (!menuItemClass) {
        JSClassDefinition classDefinition = {
            0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        classDefinition.staticFunctions = staticMenuItemFunctions;
        classDefinition.staticValues = staticMenuItemValues;

        menuItemClass = JSClassCreate(&classDefinition);
    }

    return menuItemClass;
}

static JSValueRef contextClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // Should invoke a context menu, and return its contents
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int button = 0;
    if (argumentCount == 1) {
        button = static_cast<int>(JSValueToNumber(context, arguments[0], exception));

        if (exception && *exception)
            return JSValueMakeUndefined(context);
    }

    button = translateMouseButtonNumber(button);
    // If the same mouse button is already in the down position don't send another event as it may confuse Xvfb.
    if (gButtonCurrentlyDown == button)
        return JSValueMakeUndefined(context);

    updateClickCount(button);

    unsigned modifiers = argumentCount >= 2 ? modifiersFromJSValue(context, arguments[1]) : 0;
    BMessage* eventInfo = new BMessage(B_MOUSE_DOWN);
    eventInfo->AddInt32("modifiers", modifiers);
    eventInfo->AddInt32("buttons", button);
    eventInfo->AddInt32("clicks", gClickCount);
    eventInfo->AddPoint("where", BPoint(gLastMousePositionX, gLastMousePositionY));
    eventInfo->AddPoint("be:view_where", BPoint(gLastMousePositionX, gLastMousePositionY));
    feedOrQueueMouseEvent(eventInfo, FeedQueuedEvents);
    gButtonCurrentlyDown = button;
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseUpCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int button = 0;
    if (argumentCount == 1) {
        button = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
        if (exception && *exception)
            return JSValueMakeUndefined(context);
    }

    gLastClickPositionX = gLastMousePositionX;
    gLastClickPositionY = gLastMousePositionY;
    gLastClickButton = gButtonCurrentlyDown;
    gLastClickTimeOffset = gTimeOffset;
    gButtonCurrentlyDown = 0;

    unsigned modifiers = argumentCount >= 2 ? modifiersFromJSValue(context, arguments[1]) : 0;
    BMessage* eventInfo = new BMessage(B_MOUSE_UP);
    eventInfo->AddInt32("modifiers", modifiers);
    eventInfo->AddInt32("previous buttons", gLastClickButton);
    eventInfo->AddPoint("where", BPoint(gLastMousePositionX, gLastMousePositionY));
    eventInfo->AddPoint("be:view_where", BPoint(gLastMousePositionX, gLastMousePositionY));
    feedOrQueueMouseEvent(eventInfo, FeedQueuedEvents);
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseMoveToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    gLastMousePositionX = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    if (exception && *exception)
        return JSValueMakeUndefined(context);
    gLastMousePositionY = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    if (exception && *exception)
        return JSValueMakeUndefined(context);

    BMessage* eventInfo = new BMessage(B_MOUSE_MOVED);
    eventInfo->AddPoint("where", BPoint(gLastMousePositionX, gLastMousePositionY));
    eventInfo->AddPoint("be:view_where", BPoint(gLastMousePositionX, gLastMousePositionY));
    feedOrQueueMouseEvent(eventInfo, DoNotFeedQueuedEvents);
    eventInfo->AddInt32("buttons", gButtonCurrentlyDown);
    return JSValueMakeUndefined(context);
}

static JSValueRef leapForwardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount > 0) {
        const unsigned long leapForwardDelay = JSValueToNumber(context, arguments[0], exception);
        if (delayedEventQueue().isEmpty())
            delayedEventQueue().append(new BMessage((uint32)0));
        delayedEventQueue().last()->AddInt32("delay", leapForwardDelay);
        gTimeOffset += leapForwardDelay;
    }

    return JSValueMakeUndefined(context);
}

static BMessage* evasMouseEventFromHorizontalAndVerticalOffsets(int horizontalOffset, int verticalOffset)
{
    BMessage* message = new BMessage(B_MOUSE_WHEEL_CHANGED);
    message->AddFloat("be:wheel_delta_x", horizontalOffset);
    message->AddFloat("be:wheel_delta_y", verticalOffset);
    return message;
}

static JSValueRef mouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    // We need to invert scrolling values since in EFL negative z value means that
    // canvas is scrolling down
    const int horizontal = -(static_cast<int>(JSValueToNumber(context, arguments[0], exception)));
    if (exception && *exception)
        return JSValueMakeUndefined(context);
    const int vertical = -(static_cast<int>(JSValueToNumber(context, arguments[1], exception)));
    if (exception && *exception)
        return JSValueMakeUndefined(context);

    BMessage* eventInfo = evasMouseEventFromHorizontalAndVerticalOffsets(horizontal, vertical);
    feedOrQueueMouseEvent(eventInfo, FeedQueuedEvents);
    return JSValueMakeUndefined(context);
}

static JSValueRef continuousMouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeUndefined(context);
}

static KeyEventInfo* keyPadNameFromJSValue(JSStringRef character, unsigned modifiers)
{
    if (equals(character, "leftArrow"))
        return new KeyEventInfo("KP_Left", modifiers);
    if (equals(character, "rightArrow"))
        return new KeyEventInfo("KP_Right", modifiers);
    if (equals(character, "upArrow"))
        return new KeyEventInfo("KP_Up", modifiers);
    if (equals(character, "downArrow"))
        return new KeyEventInfo("KP_Down", modifiers);
    if (equals(character, "pageUp"))
        return new KeyEventInfo("KP_Prior", modifiers);
    if (equals(character, "pageDown"))
        return new KeyEventInfo("KP_Next", modifiers);
    if (equals(character, "home"))
        return new KeyEventInfo("KP_Home", modifiers);
    if (equals(character, "end"))
        return new KeyEventInfo("KP_End", modifiers);
    if (equals(character, "insert"))
        return new KeyEventInfo("KP_Insert", modifiers);
    if (equals(character, "delete"))
        return new KeyEventInfo("KP_Delete", modifiers);

    return new KeyEventInfo(character->string().utf8(), modifiers, character->string().utf8());
}

static KeyEventInfo* keyNameFromJSValue(JSStringRef character, unsigned modifiers)
{
    if (equals(character, "leftArrow"))
        return new KeyEventInfo("Left", modifiers);
    if (equals(character, "rightArrow"))
        return new KeyEventInfo("Right", modifiers);
    if (equals(character, "upArrow"))
        return new KeyEventInfo("Up", modifiers);
    if (equals(character, "downArrow"))
        return new KeyEventInfo("Down", modifiers);
    if (equals(character, "pageUp"))
        return new KeyEventInfo("Prior", modifiers);
    if (equals(character, "pageDown"))
        return new KeyEventInfo("Next", modifiers);
    if (equals(character, "home"))
        return new KeyEventInfo("Home", modifiers);
    if (equals(character, "end"))
        return new KeyEventInfo("End", modifiers);
    if (equals(character, "insert"))
        return new KeyEventInfo("Insert", modifiers);
    if (equals(character, "delete"))
        return new KeyEventInfo("Delete", modifiers);
    if (equals(character, "printScreen"))
        return new KeyEventInfo("Print", modifiers);
    if (equals(character, "menu"))
        return new KeyEventInfo("Menu", modifiers);
    if (equals(character, "leftControl"))
        return new KeyEventInfo("Control_L", modifiers);
    if (equals(character, "rightControl"))
        return new KeyEventInfo("Control_R", modifiers);
    if (equals(character, "leftShift"))
        return new KeyEventInfo("Shift_L", modifiers);
    if (equals(character, "rightShift"))
        return new KeyEventInfo("Shift_R", modifiers);
    if (equals(character, "leftAlt"))
        return new KeyEventInfo("Alt_L", modifiers);
    if (equals(character, "rightAlt"))
        return new KeyEventInfo("Alt_R", modifiers);
    if (equals(character, "F1"))
        return new KeyEventInfo("F1", modifiers);
    if (equals(character, "F2"))
        return new KeyEventInfo("F2", modifiers);
    if (equals(character, "F3"))
        return new KeyEventInfo("F3", modifiers);
    if (equals(character, "F4"))
        return new KeyEventInfo("F4", modifiers);
    if (equals(character, "F5"))
        return new KeyEventInfo("F5", modifiers);
    if (equals(character, "F6"))
        return new KeyEventInfo("F6", modifiers);
    if (equals(character, "F7"))
        return new KeyEventInfo("F7", modifiers);
    if (equals(character, "F8"))
        return new KeyEventInfo("F8", modifiers);
    if (equals(character, "F9"))
        return new KeyEventInfo("F9", modifiers);
    if (equals(character, "F10"))
        return new KeyEventInfo("F10", modifiers);
    if (equals(character, "F11"))
        return new KeyEventInfo("F11", modifiers);
    if (equals(character, "F12"))
        return new KeyEventInfo("F12", modifiers);

    int charCode = JSStringGetCharactersPtr(character)[0];
    if (charCode == '\n' || charCode == '\r')
        return new KeyEventInfo("Return", modifiers, "\r");
    if (charCode == '\t')
        return new KeyEventInfo("Tab", modifiers, "\t");
    if (charCode == '\x8')
        return new KeyEventInfo("BackSpace", modifiers, "\x8");
    if (charCode == ' ')
        return new KeyEventInfo("space", modifiers, " ");
    if (charCode == '\x1B')
        return new KeyEventInfo("Escape", modifiers, "\x1B");

    if ((character->length() == 1) && (charCode >= 'A' && charCode <= 'Z'))
        modifiers |= B_SHIFT_KEY;

    return new KeyEventInfo(character->string().utf8(), modifiers, character->string().utf8());
}

static KeyEventInfo* createKeyEventInfo(JSContextRef context, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return 0;
}

static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSValueRef scalePageByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static void textZoom(ZoomEvent zoomEvent)
{
    notImplemented();
}

static void pageZoom(ZoomEvent zoomEvent)
{
    notImplemented();
}

static JSValueRef textZoomInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    textZoom(ZoomIn);
    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    textZoom(ZoomOut);
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    pageZoom(ZoomIn);
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    pageZoom(ZoomOut);
    return JSValueMakeUndefined(context);
}

static JSValueRef scheduleAsynchronousKeyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseScrollBy", mouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#if 0
    { "contextClick", contextClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "continuousMouseScrollBy", continuousMouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "beginDragWithFiles", beginDragWithFilesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "leapForward", leapForwardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scheduleAsynchronousClick", scheduleAsynchronousClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scheduleAsynchronousKeyDown", scheduleAsynchronousKeyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scalePageBy", scalePageByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomIn", textZoomInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomOut", textZoomOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageIn", zoomPageInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageOut", zoomPageOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
#endif
    { 0, 0, 0 }
};

static JSStaticValue staticValues[] = {
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

JSObjectRef makeEventSender(JSContextRef context)
{
    return JSObjectMake(context, getClass(context), 0);
}

extern BWebView* webView;

static void feedOrQueueMouseEvent(BMessage* eventInfo, EventQueueStrategy strategy)
{
    if (!delayedEventQueue().isEmpty()) {
        if (delayedEventQueue().last() != NULL)
            delayedEventQueue().append(eventInfo);
        else
            delayedEventQueue().last() = eventInfo;

        if (strategy == FeedQueuedEvents)
            feedQueuedMouseEvents();
    } else
        WebCore::DumpRenderTreeClient::injectMouseEvent(webView->WebPage(), eventInfo);
}

namespace WebCore {

void DumpRenderTreeClient::injectMouseEvent(BWebPage* target, BMessage* event)
{
    // We are short-circuiting the normal message delivery path, because tests
    // expect this to be synchronous (the event must be processed when the
    // method returns)
    target->handleMouseEvent(event);
    delete event;
}

}

static void feedQueuedMouseEvents()
{
    WTF::Vector<BMessage*>::const_iterator it = delayedEventQueue().begin();
    for (; it != delayedEventQueue().end(); it++) {
        BMessage* delayedEvent = *it;
        int32 delay = delayedEvent->FindInt32("delay");
        if (delay)
            usleep(delay * 1000);
        WebCore::DumpRenderTreeClient::injectMouseEvent(webView->WebPage(), delayedEvent);
    }
    delayedEventQueue().clear();
}
