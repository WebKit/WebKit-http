/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
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
#include "DumpRenderTreeChrome.h"
#include "JSStringUtils.h"
#include "NotImplemented.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"
#include "ewk_private.h"
#include <EWebKit.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <wtf/ASCIICType.h>
#include <wtf/Platform.h>
#include <wtf/text/CString.h>

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

enum EvasKeyModifier {
    EvasKeyModifierNone    = 0,
    EvasKeyModifierControl = 1 << 0,
    EvasKeyModifierShift   = 1 << 1,
    EvasKeyModifierAlt     = 1 << 2,
    EvasKeyModifierMeta    = 1 << 3
};

enum EvasMouseButton {
    EvasMouseButtonNone,
    EvasMouseButtonLeft,
    EvasMouseButtonMiddle,
    EvasMouseButtonRight
};

enum EvasMouseEvent {
    EvasMouseEventNone        = 0,
    EvasMouseEventDown        = 1 << 0,
    EvasMouseEventUp          = 1 << 1,
    EvasMouseEventMove        = 1 << 2,
    EvasMouseEventScrollUp    = 1 << 3,
    EvasMouseEventScrollDown  = 1 << 4,
    EvasMouseEventScrollLeft  = 1 << 5,
    EvasMouseEventScrollRight = 1 << 6,
    EvasMouseEventClick       = EvasMouseEventMove | EvasMouseEventDown | EvasMouseEventUp,
};

static void setEvasModifiers(Evas* evas, EvasKeyModifier modifiers)
{
    static const char* modifierNames[] = { "Control", "Shift", "Alt", "Super" };
    for (unsigned modifier = 0; modifier < 4; ++modifier) {
        if (modifiers & (1 << modifier))
            evas_key_modifier_on(evas, modifierNames[modifier]);
        else
            evas_key_modifier_off(evas, modifierNames[modifier]);
    }
}

static EvasMouseButton translateMouseButtonNumber(int eventSenderButtonNumber)
{
    static const EvasMouseButton translationTable[] = {
        EvasMouseButtonLeft,
        EvasMouseButtonMiddle,
        EvasMouseButtonRight,
        EvasMouseButtonMiddle // fast/events/mouse-click-events expects the 4th button to be treated as the middle button
    };
    static const unsigned translationTableSize = sizeof(translationTable) / sizeof(translationTable[0]);

    if (eventSenderButtonNumber < translationTableSize)
        return translationTable[eventSenderButtonNumber];

    return EvasMouseButtonLeft;
}

static bool sendMouseEvent(Evas* evas, EvasMouseEvent event, int buttonNumber, EvasKeyModifier modifiers)
{
    unsigned timeStamp = 0;

    DumpRenderTreeSupportEfl::layoutFrame(browser->mainFrame());

    setEvasModifiers(evas, modifiers);
    if (event & EvasMouseEventMove)
        evas_event_feed_mouse_move(evas, gLastMousePositionX, gLastMousePositionY, timeStamp++, 0);
    if (event & EvasMouseEventDown)
        evas_event_feed_mouse_down(evas, buttonNumber, EVAS_BUTTON_NONE, timeStamp++, 0);
    if (event & EvasMouseEventUp)
        evas_event_feed_mouse_up(evas, buttonNumber, EVAS_BUTTON_NONE, timeStamp++, 0);

    const bool horizontal = !!(event & EvasMouseEventScrollLeft | event & EvasMouseEventScrollRight);
    const bool vertical = !!(event & EvasMouseEventScrollUp | event & EvasMouseEventScrollDown);
    if (vertical && horizontal) {
        evas_event_feed_mouse_wheel(evas, 0, (event & EvasMouseEventScrollUp) ? 10 : -10, timeStamp, 0);
        evas_event_feed_mouse_wheel(evas, 1, (event & EvasMouseEventScrollLeft) ? 10 : -10, timeStamp, 0);
    } else if (vertical)
        evas_event_feed_mouse_wheel(evas, 0, (event & EvasMouseEventScrollUp) ? 10 : -10, timeStamp, 0);
    else if (horizontal)
        evas_event_feed_mouse_wheel(evas, 1, (event & EvasMouseEventScrollLeft) ? 10 : -10, timeStamp, 0);

    setEvasModifiers(evas, EvasKeyModifierNone);

    return true;
}

static Eina_Bool sendClick(void*)
{
    return !!sendMouseEvent(evas_object_evas_get(browser->mainFrame()), EvasMouseEventClick, EvasMouseButtonLeft, EvasKeyModifierNone);
}

static JSValueRef scheduleAsynchronousClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ecore_idler_add(sendClick, 0);
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

static EvasKeyModifier modifierFromJSValue(JSContextRef context, const JSValueRef value)
{
    JSRetainPtr<JSStringRef> jsKeyValue(Adopt, JSValueToStringCopy(context, value, 0));

    if (equals(jsKeyValue, "ctrlKey") || equals(jsKeyValue, "addSelectionKey"))
        return EvasKeyModifierControl;
    if (equals(jsKeyValue, "shiftKey") || equals(jsKeyValue, "rangeSelectionKey"))
        return EvasKeyModifierShift;
    if (equals(jsKeyValue, "altKey"))
        return EvasKeyModifierAlt;
    if (equals(jsKeyValue, "metaKey"))
        return EvasKeyModifierMeta;

    return EvasKeyModifierNone;
}

static EvasKeyModifier modifiersFromJSValue(JSContextRef context, const JSValueRef modifiers)
{
    // The value may either be a string with a single modifier or an array of modifiers.
    if (JSValueIsString(context, modifiers))
        return modifierFromJSValue(context, modifiers);

    JSObjectRef modifiersArray = JSValueToObject(context, modifiers, 0);
    if (!modifiersArray)
        return EvasKeyModifierNone;

    unsigned modifier = 0;
    int modifiersCount = JSValueToNumber(context, JSObjectGetProperty(context, modifiersArray, JSStringCreateWithUTF8CString("length"), 0), 0);
    for (int i = 0; i < modifiersCount; ++i)
        modifier |= static_cast<unsigned>(modifierFromJSValue(context, JSObjectGetPropertyAtIndex(context, modifiersArray, i, 0)));
    return static_cast<EvasKeyModifier>(modifier);
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

    EvasKeyModifier modifiers = argumentCount >= 2 ? modifiersFromJSValue(context, arguments[1]) : EvasKeyModifierNone;
    if (!sendMouseEvent(evas_object_evas_get(browser->mainFrame()), EvasMouseEventDown, button, modifiers))
        return JSValueMakeUndefined(context);

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

    EvasKeyModifier modifiers = argumentCount >= 2 ? modifiersFromJSValue(context, arguments[1]) : EvasKeyModifierNone;
    sendMouseEvent(evas_object_evas_get(browser->mainFrame()), EvasMouseEventUp, translateMouseButtonNumber(button), modifiers);
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

    sendMouseEvent(evas_object_evas_get(browser->mainFrame()), EvasMouseEventMove, EvasMouseButtonNone, EvasKeyModifierNone);
    return JSValueMakeUndefined(context);
}

static EvasMouseEvent evasMouseEventFromHorizontalAndVerticalOffsets(int horizontalOffset, int verticalOffset)
{
    unsigned mouseEvent = 0;

    if (verticalOffset > 0)
        mouseEvent |= EvasMouseEventScrollUp;
    else if (verticalOffset < 0)
        mouseEvent |= EvasMouseEventScrollDown;

    if (horizontalOffset > 0)
        mouseEvent |= EvasMouseEventScrollRight;
    else if (horizontalOffset < 0)
        mouseEvent |= EvasMouseEventScrollLeft;

    return static_cast<EvasMouseEvent>(mouseEvent);
}

static JSValueRef mouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    const int horizontal = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    if (exception && *exception)
        return JSValueMakeUndefined(context);
    const int vertical = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    if (exception && *exception)
        return JSValueMakeUndefined(context);

    sendMouseEvent(evas_object_evas_get(browser->mainFrame()), evasMouseEventFromHorizontalAndVerticalOffsets(horizontal, vertical), EvasMouseButtonNone, EvasKeyModifierNone);
    return JSValueMakeUndefined(context);
}

static JSValueRef continuousMouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeUndefined(context);
}

static const char* keyPadNameFromJSValue(JSStringRef character)
{
    if (equals(character, "leftArrow"))
        return "KP_Left";
    if (equals(character, "rightArrow"))
        return "KP_Right";
    if (equals(character, "upArrow"))
        return "KP_Up";
    if (equals(character, "downArrow"))
        return "KP_Down";
    if (equals(character, "pageUp"))
        return "KP_Prior";
    if (equals(character, "pageDown"))
        return "KP_Next";
    if (equals(character, "home"))
        return "KP_Home";
    if (equals(character, "end"))
        return "KP_End";
    if (equals(character, "insert"))
        return "KP_Insert";
    if (equals(character, "delete"))
        return "KP_Delete";

    return 0;
}

static const char* keyNameFromJSValue(JSStringRef character)
{
    if (equals(character, "leftArrow"))
        return "Left";
    if (equals(character, "rightArrow"))
        return "Right";
    if (equals(character, "upArrow"))
        return "Up";
    if (equals(character, "downArrow"))
        return "Down";
    if (equals(character, "pageUp"))
        return "Prior";
    if (equals(character, "pageDown"))
        return "Next";
    if (equals(character, "home"))
        return "Home";
    if (equals(character, "end"))
        return "End";
    if (equals(character, "insert"))
        return "Insert";
    if (equals(character, "delete"))
        return "Delete";
    if (equals(character, "printScreen"))
        return "Print";
    if (equals(character, "menu"))
        return "Menu";
    if (equals(character, "F1"))
        return "F1";
    if (equals(character, "F2"))
        return "F2";
    if (equals(character, "F3"))
        return "F3";
    if (equals(character, "F4"))
        return "F4";
    if (equals(character, "F5"))
        return "F5";
    if (equals(character, "F6"))
        return "F6";
    if (equals(character, "F7"))
        return "F7";
    if (equals(character, "F8"))
        return "F8";
    if (equals(character, "F9"))
        return "F9";
    if (equals(character, "F10"))
        return "F10";
    if (equals(character, "F11"))
        return "F11";
    if (equals(character, "F12"))
        return "F12";

    int charCode = JSStringGetCharactersPtr(character)[0];
    if (charCode == '\n' || charCode == '\r')
        return "Return";
    if (charCode == '\t')
        return "Tab";
    if (charCode == '\x8')
        return "BackSpace";

    return 0;
}

static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* view = ewk_frame_view_get(browser->mainFrame());
    if (!view)
        return JSValueMakeUndefined(context);

    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    // handle location argument.
    int location = DomKeyLocationStandard;
    if (argumentCount > 2)
        location = static_cast<int>(JSValueToNumber(context, arguments[2], exception));

    JSRetainPtr<JSStringRef> character(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    if (exception && *exception)
        return JSValueMakeUndefined(context);

    // send the event
    Evas* evas = evas_object_evas_get(view);
    if (argumentCount >= 2)
        setEvasModifiers(evas, modifiersFromJSValue(context, arguments[1]));

    const CString cCharacter = character.get()->ustring().utf8();
    const char* keyName = (location == DomKeyLocationNumpad) ? keyPadNameFromJSValue(character.get()) : keyNameFromJSValue(character.get());

    if (!keyName)
        keyName = cCharacter.data();

    DumpRenderTreeSupportEfl::layoutFrame(browser->mainFrame());
    evas_event_feed_key_down(evas, keyName, keyName, keyName, 0, 0, 0);
    evas_event_feed_key_up(evas, keyName, keyName, keyName, 0, 1, 0);

    setEvasModifiers(evas, EvasKeyModifierNone);

    return JSValueMakeUndefined(context);
}

static JSValueRef scalePageByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    Evas_Object* view = ewk_frame_view_get(browser->mainFrame());
    if (!view)
        return JSValueMakeUndefined(context);

    float scaleFactor = JSValueToNumber(context, arguments[0], exception);
    float x = JSValueToNumber(context, arguments[1], exception);
    float y = JSValueToNumber(context, arguments[2], exception);
    ewk_view_scale_set(view, scaleFactor, x, y);

    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "mouseScrollBy", mouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "continuousMouseScrollBy", continuousMouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scheduleAsynchronousClick", scheduleAsynchronousClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scalePageBy", scalePageByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
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

JSObjectRef makeEventSender(JSContextRef context, bool isTopFrame)
{
    if (isTopFrame) {
        gDragMode = true;

        // Fly forward in time one second when the main frame loads. This will
        // ensure that when a test begins clicking in the same location as
        // a previous test, those clicks won't be interpreted as continuations
        // of the previous test's click sequences.
        gTimeOffset += 1000;

        gLastMousePositionX = gLastMousePositionY = 0;
        gLastClickPositionX = gLastClickPositionY = 0;
        gLastClickTimeOffset = 0;
        gLastClickButton = 0;
        gButtonCurrentlyDown = 0;
        gClickCount = 0;
    }

    return JSObjectMake(context, getClass(context), 0);
}
