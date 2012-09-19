/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

// This file contains the definition for EventSender.
//
// Some notes about drag and drop handling:
// Windows drag and drop goes through a system call to doDragDrop. At that
// point, program control is given to Windows which then periodically makes
// callbacks into the webview. This won't work for layout tests, so instead,
// we queue up all the mouse move and mouse up events. When the test tries to
// start a drag (by calling EvenSendingController::doDragDrop), we take the
// events in the queue and replay them.
// The behavior of queuing events and replaying them can be disabled by a
// layout test by setting eventSender.dragMode to false.

#include "config.h"
#include "EventSender.h"

#include "TestDelegate.h"
#include "WebContextMenuData.h"
#include "WebDragOperation.h"
#include "WebTouchPoint.h"
#include "WebView.h"
#include "platform/WebDragData.h"
#include "platform/WebPoint.h"
#include "platform/WebString.h"
#include "webkit/support/webkit_support.h"
#include <wtf/Deque.h>
#include <wtf/StringExtras.h>

#if OS(WINDOWS)
#include "win/WebInputEventFactory.h"
#endif

// FIXME: layout before each event?

using namespace std;
using namespace WebKit;

WebPoint EventSender::lastMousePos;
WebMouseEvent::Button EventSender::pressedButton = WebMouseEvent::ButtonNone;
WebMouseEvent::Button EventSender::lastButtonType = WebMouseEvent::ButtonNone;

struct SavedEvent {
    enum SavedEventType {
        Unspecified,
        MouseUp,
        MouseMove,
        LeapForward
    };

    SavedEventType type;
    WebMouseEvent::Button buttonType; // For MouseUp.
    WebPoint pos; // For MouseMove.
    int milliseconds; // For LeapForward.

    SavedEvent()
        : type(Unspecified)
        , buttonType(WebMouseEvent::ButtonNone)
        , milliseconds(0) { }
};

static WebDragData currentDragData;
static WebDragOperation currentDragEffect;
static WebDragOperationsMask currentDragEffectsAllowed;
static bool replayingSavedEvents = false;
static Deque<SavedEvent> mouseEventQueue;
static int touchModifiers;
static Vector<WebTouchPoint> touchPoints;

// Time and place of the last mouse up event.
static double lastClickTimeSec = 0;
static WebPoint lastClickPos;
static int clickCount = 0;

// maximum distance (in space and time) for a mouse click
// to register as a double or triple click
static const double multipleClickTimeSec = 1;
static const int multipleClickRadiusPixels = 5;

// How much we should scroll per event - the value here is chosen to
// match the WebKit impl and layout test results.
static const float scrollbarPixelsPerTick = 40.0f;

inline bool outsideMultiClickRadius(const WebPoint& a, const WebPoint& b)
{
    return ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)) >
        multipleClickRadiusPixels * multipleClickRadiusPixels;
}

// Used to offset the time the event hander things an event happened. This is
// done so tests can run without a delay, but bypass checks that are time
// dependent (e.g., dragging has a timeout vs selection).
static uint32 timeOffsetMs = 0;

static double getCurrentEventTimeSec()
{
    return (webkit_support::GetCurrentTimeInMillisecond() + timeOffsetMs) / 1000.0;
}

static void advanceEventTime(int32_t deltaMs)
{
    timeOffsetMs += deltaMs;
}

static void initMouseEvent(WebInputEvent::Type t, WebMouseEvent::Button b,
                           const WebPoint& pos, WebMouseEvent* e)
{
    e->type = t;
    e->button = b;
    e->modifiers = 0;
    e->x = pos.x;
    e->y = pos.y;
    e->globalX = pos.x;
    e->globalY = pos.y;
    e->timeStampSeconds = getCurrentEventTimeSec();
    e->clickCount = clickCount;
}

// Returns true if the specified key is the system key.
static bool applyKeyModifier(const string& modifierName, WebInputEvent* event)
{
    bool isSystemKey = false;
    const char* characters = modifierName.c_str();
    if (!strcmp(characters, "ctrlKey")
#if !OS(MAC_OS_X)
        || !strcmp(characters, "addSelectionKey")
#endif
        ) {
        event->modifiers |= WebInputEvent::ControlKey;
    } else if (!strcmp(characters, "shiftKey") || !strcmp(characters, "rangeSelectionKey"))
        event->modifiers |= WebInputEvent::ShiftKey;
    else if (!strcmp(characters, "altKey")) {
        event->modifiers |= WebInputEvent::AltKey;
#if !OS(MAC_OS_X)
        // On Windows all keys with Alt modifier will be marked as system key.
        // We keep the same behavior on Linux and everywhere non-Mac, see:
        // WebKit/chromium/src/gtk/WebInputEventFactory.cpp
        // If we want to change this behavior on Linux, this piece of code must be
        // kept in sync with the related code in above file.
        isSystemKey = true;
#endif
#if OS(MAC_OS_X)
    } else if (!strcmp(characters, "metaKey") || !strcmp(characters, "addSelectionKey")) {
        event->modifiers |= WebInputEvent::MetaKey;
        // On Mac only command key presses are marked as system key.
        // See the related code in: WebKit/chromium/src/mac/WebInputEventFactory.cpp
        // It must be kept in sync with the related code in above file.
        isSystemKey = true;
#else
    } else if (!strcmp(characters, "metaKey")) {
        event->modifiers |= WebInputEvent::MetaKey;
#endif
    }
    return isSystemKey;
}

static bool applyKeyModifiers(const CppVariant* argument, WebInputEvent* event)
{
    bool isSystemKey = false;
    if (argument->isObject()) {
        Vector<string> modifiers = argument->toStringVector();
        for (Vector<string>::const_iterator i = modifiers.begin(); i != modifiers.end(); ++i)
            isSystemKey |= applyKeyModifier(*i, event);
    } else if (argument->isString())
        isSystemKey = applyKeyModifier(argument->toString(), event);
    return isSystemKey;
}

// Get the edit command corresponding to a keyboard event.
// Returns true if the specified event corresponds to an edit command, the name
// of the edit command will be stored in |*name|.
bool getEditCommand(const WebKeyboardEvent& event, string* name)
{
#if OS(MAC_OS_X)
    // We only cares about Left,Right,Up,Down keys with Command or Command+Shift
    // modifiers. These key events correspond to some special movement and
    // selection editor commands, and was supposed to be handled in
    // WebKit/chromium/src/EditorClientImpl.cpp. But these keys will be marked
    // as system key, which prevents them from being handled. Thus they must be
    // handled specially.
    if ((event.modifiers & ~WebKeyboardEvent::ShiftKey) != WebKeyboardEvent::MetaKey)
        return false;

    switch (event.windowsKeyCode) {
    case webkit_support::VKEY_LEFT:
        *name = "MoveToBeginningOfLine";
        break;
    case webkit_support::VKEY_RIGHT:
        *name = "MoveToEndOfLine";
        break;
    case webkit_support::VKEY_UP:
        *name = "MoveToBeginningOfDocument";
        break;
    case webkit_support::VKEY_DOWN:
        *name = "MoveToEndOfDocument";
        break;
    default:
        return false;
    }

    if (event.modifiers & WebKeyboardEvent::ShiftKey)
        name->append("AndModifySelection");

    return true;
#else
    return false;
#endif
}

// Key event location code introduced in DOM Level 3.
// See also: http://www.w3.org/TR/DOM-Level-3-Events/#events-keyboardevents
enum KeyLocationCode {
    DOMKeyLocationStandard      = 0x00,
    DOMKeyLocationLeft          = 0x01,
    DOMKeyLocationRight         = 0x02,
    DOMKeyLocationNumpad        = 0x03
};

EventSender::EventSender()
    : m_delegate(0)
{
    // Initialize the map that associates methods of this class with the names
    // they will use when called by JavaScript. The actual binding of those
    // names to their methods will be done by calling bindToJavaScript() (defined
    // by CppBoundClass, the parent to EventSender).
    bindMethod("addTouchPoint", &EventSender::addTouchPoint);
    bindMethod("beginDragWithFiles", &EventSender::beginDragWithFiles);
    bindMethod("cancelTouchPoint", &EventSender::cancelTouchPoint);
    bindMethod("clearKillRing", &EventSender::clearKillRing);
    bindMethod("clearTouchPoints", &EventSender::clearTouchPoints);
    bindMethod("contextClick", &EventSender::contextClick);
    bindMethod("continuousMouseScrollBy", &EventSender::continuousMouseScrollBy);
    bindMethod("dispatchMessage", &EventSender::dispatchMessage);
    bindMethod("dumpFilenameBeingDragged", &EventSender::dumpFilenameBeingDragged);
    bindMethod("enableDOMUIEventLogging", &EventSender::enableDOMUIEventLogging);
    bindMethod("fireKeyboardEventsToElement", &EventSender::fireKeyboardEventsToElement);
    bindMethod("keyDown", &EventSender::keyDown);
    bindMethod("leapForward", &EventSender::leapForward);
    bindMethod("mouseDown", &EventSender::mouseDown);
    bindMethod("mouseMoveTo", &EventSender::mouseMoveTo);
    bindMethod("mouseScrollBy", &EventSender::mouseScrollBy);
    bindMethod("mouseUp", &EventSender::mouseUp);
    bindMethod("releaseTouchPoint", &EventSender::releaseTouchPoint);
    bindMethod("scheduleAsynchronousClick", &EventSender::scheduleAsynchronousClick);
    bindMethod("scheduleAsynchronousKeyDown", &EventSender::scheduleAsynchronousKeyDown);
    bindMethod("setTouchModifier", &EventSender::setTouchModifier);
    bindMethod("textZoomIn", &EventSender::textZoomIn);
    bindMethod("textZoomOut", &EventSender::textZoomOut);
    bindMethod("touchCancel", &EventSender::touchCancel);
    bindMethod("touchEnd", &EventSender::touchEnd);
    bindMethod("touchMove", &EventSender::touchMove);
    bindMethod("touchStart", &EventSender::touchStart);
    bindMethod("updateTouchPoint", &EventSender::updateTouchPoint);
    bindMethod("gestureFlingCancel", &EventSender::gestureFlingCancel);
    bindMethod("gestureFlingStart", &EventSender::gestureFlingStart);
    bindMethod("gestureScrollBegin", &EventSender::gestureScrollBegin);
    bindMethod("gestureScrollEnd", &EventSender::gestureScrollEnd);
    bindMethod("gestureScrollFirstPoint", &EventSender::gestureScrollFirstPoint);
    bindMethod("gestureScrollUpdate", &EventSender::gestureScrollUpdate);
    bindMethod("gestureTap", &EventSender::gestureTap);
    bindMethod("gestureTapDown", &EventSender::gestureTapDown);
    bindMethod("gestureTapCancel", &EventSender::gestureTapCancel);
    bindMethod("gestureLongPress", &EventSender::gestureLongPress);
    bindMethod("gestureTwoFingerTap", &EventSender::gestureTwoFingerTap);
    bindMethod("zoomPageIn", &EventSender::zoomPageIn);
    bindMethod("zoomPageOut", &EventSender::zoomPageOut);
    bindMethod("scalePageBy", &EventSender::scalePageBy);

    // When set to true (the default value), we batch mouse move and mouse up
    // events so we can simulate drag & drop.
    bindProperty("dragMode", &dragMode);
#if OS(WINDOWS)
    bindProperty("WM_KEYDOWN", &wmKeyDown);
    bindProperty("WM_KEYUP", &wmKeyUp);
    bindProperty("WM_CHAR", &wmChar);
    bindProperty("WM_DEADCHAR", &wmDeadChar);
    bindProperty("WM_SYSKEYDOWN", &wmSysKeyDown);
    bindProperty("WM_SYSKEYUP", &wmSysKeyUp);
    bindProperty("WM_SYSCHAR", &wmSysChar);
    bindProperty("WM_SYSDEADCHAR", &wmSysDeadChar);
#endif
}

void EventSender::reset()
{
    // The test should have finished a drag and the mouse button state.
    ASSERT(currentDragData.isNull());
    currentDragData.reset();
    currentDragEffect = WebKit::WebDragOperationNone;
    currentDragEffectsAllowed = WebKit::WebDragOperationNone;
    pressedButton = WebMouseEvent::ButtonNone;
    dragMode.set(true);
#if OS(WINDOWS)
    wmKeyDown.set(WM_KEYDOWN);
    wmKeyUp.set(WM_KEYUP);
    wmChar.set(WM_CHAR);
    wmDeadChar.set(WM_DEADCHAR);
    wmSysKeyDown.set(WM_SYSKEYDOWN);
    wmSysKeyUp.set(WM_SYSKEYUP);
    wmSysChar.set(WM_SYSCHAR);
    wmSysDeadChar.set(WM_SYSDEADCHAR);
#endif
    lastMousePos = WebPoint(0, 0);
    lastClickTimeSec = 0;
    lastClickPos = WebPoint(0, 0);
    clickCount = 0;
    lastButtonType = WebMouseEvent::ButtonNone;
    timeOffsetMs = 0;
    touchModifiers = 0;
    touchPoints.clear();
    m_taskList.revokeAll();
    m_currentGestureLocation = WebPoint(0, 0);
}

void EventSender::doDragDrop(const WebDragData& dragData, WebDragOperationsMask mask)
{
    WebMouseEvent event;
    initMouseEvent(WebInputEvent::MouseDown, pressedButton, lastMousePos, &event);
    WebPoint clientPoint(event.x, event.y);
    WebPoint screenPoint(event.globalX, event.globalY);
    currentDragData = dragData;
    currentDragEffectsAllowed = mask;
    currentDragEffect = webview()->dragTargetDragEnter(dragData, clientPoint, screenPoint, currentDragEffectsAllowed, 0);

    // Finish processing events.
    replaySavedEvents();
}

void EventSender::dumpFilenameBeingDragged(const CppArgumentList&, CppVariant*)
{
    WebString filename;
    WebVector<WebDragData::Item> items = currentDragData.items();
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].storageType == WebDragData::Item::StorageTypeBinaryData) {
            filename = items[i].title;
            break;
        }
    }
    printf("Filename being dragged: %s\n", filename.utf8().data());
}

WebMouseEvent::Button EventSender::getButtonTypeFromButtonNumber(int buttonCode)
{
    if (!buttonCode)
        return WebMouseEvent::ButtonLeft;
    if (buttonCode == 2)
        return WebMouseEvent::ButtonRight;
    return WebMouseEvent::ButtonMiddle;
}

int EventSender::getButtonNumberFromSingleArg(const CppArgumentList& arguments)
{
    int buttonCode = 0;
    if (arguments.size() > 0 && arguments[0].isNumber())
        buttonCode = arguments[0].toInt32();
    return buttonCode;
}

void EventSender::updateClickCountForButton(WebMouseEvent::Button buttonType)
{
    if ((getCurrentEventTimeSec() - lastClickTimeSec < multipleClickTimeSec)
        && (!outsideMultiClickRadius(lastMousePos, lastClickPos))
        && (buttonType == lastButtonType))
        ++clickCount;
    else {
        clickCount = 1;
        lastButtonType = buttonType;
    }
}

//
// Implemented javascript methods.
//

void EventSender::mouseDown(const CppArgumentList& arguments, CppVariant* result)
{
    if (result) // Could be 0 if invoked asynchronously.
        result->setNull();

    webview()->layout();

    int buttonNumber = getButtonNumberFromSingleArg(arguments);
    ASSERT(buttonNumber != -1);

    WebMouseEvent::Button buttonType = getButtonTypeFromButtonNumber(buttonNumber);

    updateClickCountForButton(buttonType);

    WebMouseEvent event;
    pressedButton = buttonType;
    initMouseEvent(WebInputEvent::MouseDown, buttonType, lastMousePos, &event);
    if (arguments.size() >= 2 && (arguments[1].isObject() || arguments[1].isString()))
        applyKeyModifiers(&(arguments[1]), &event);
    webview()->handleInputEvent(event);
}

void EventSender::mouseUp(const CppArgumentList& arguments, CppVariant* result)
{
    if (result) // Could be 0 if invoked asynchronously.
        result->setNull();

    webview()->layout();

    int buttonNumber = getButtonNumberFromSingleArg(arguments);
    ASSERT(buttonNumber != -1);

    WebMouseEvent::Button buttonType = getButtonTypeFromButtonNumber(buttonNumber);

    if (isDragMode() && !replayingSavedEvents) {
        SavedEvent savedEvent;
        savedEvent.type = SavedEvent::MouseUp;
        savedEvent.buttonType = buttonType;
        mouseEventQueue.append(savedEvent);
        replaySavedEvents();
    } else {
        WebMouseEvent event;
        initMouseEvent(WebInputEvent::MouseUp, buttonType, lastMousePos, &event);
        if (arguments.size() >= 2 && (arguments[1].isObject() || arguments[1].isString()))
            applyKeyModifiers(&(arguments[1]), &event);
        doMouseUp(event);
    }
}

void EventSender::doMouseUp(const WebMouseEvent& e)
{
    webview()->handleInputEvent(e);

    pressedButton = WebMouseEvent::ButtonNone;
    lastClickTimeSec = e.timeStampSeconds;
    lastClickPos = lastMousePos;

    // If we're in a drag operation, complete it.
    if (currentDragData.isNull())
        return;
    WebPoint clientPoint(e.x, e.y);
    WebPoint screenPoint(e.globalX, e.globalY);

    currentDragEffect = webview()->dragTargetDragOver(clientPoint, screenPoint, currentDragEffectsAllowed, 0);
    if (currentDragEffect)
        webview()->dragTargetDrop(clientPoint, screenPoint, 0);
    else
        webview()->dragTargetDragLeave();
    webview()->dragSourceEndedAt(clientPoint, screenPoint, currentDragEffect);
    webview()->dragSourceSystemDragEnded();

    currentDragData.reset();
}

void EventSender::mouseMoveTo(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    if (arguments.size() < 2 || !arguments[0].isNumber() || !arguments[1].isNumber())
        return;
    webview()->layout();

    WebPoint mousePos(arguments[0].toInt32(), arguments[1].toInt32());

    if (isDragMode() && pressedButton == WebMouseEvent::ButtonLeft && !replayingSavedEvents) {
        SavedEvent savedEvent;
        savedEvent.type = SavedEvent::MouseMove;
        savedEvent.pos = mousePos;
        mouseEventQueue.append(savedEvent);
    } else {
        WebMouseEvent event;
        initMouseEvent(WebInputEvent::MouseMove, pressedButton, mousePos, &event);
        doMouseMove(event);
    }
}

void EventSender::doMouseMove(const WebMouseEvent& e)
{
    lastMousePos = WebPoint(e.x, e.y);

    webview()->handleInputEvent(e);

    if (pressedButton == WebMouseEvent::ButtonNone || currentDragData.isNull())
        return;
    WebPoint clientPoint(e.x, e.y);
    WebPoint screenPoint(e.globalX, e.globalY);
    currentDragEffect = webview()->dragTargetDragOver(clientPoint, screenPoint, currentDragEffectsAllowed, 0);
}

void EventSender::keyDown(const CppArgumentList& arguments, CppVariant* result)
{
    if (result)
        result->setNull();
    if (arguments.size() < 1 || !arguments[0].isString())
        return;
    bool generateChar = false;

    // FIXME: I'm not exactly sure how we should convert the string to a key
    // event. This seems to work in the cases I tested.
    // FIXME: Should we also generate a KEY_UP?
    string codeStr = arguments[0].toString();

    // Convert \n -> VK_RETURN. Some layout tests use \n to mean "Enter", when
    // Windows uses \r for "Enter".
    int code = 0;
    int text = 0;
    bool needsShiftKeyModifier = false;
    if ("\n" == codeStr) {
        generateChar = true;
        text = code = webkit_support::VKEY_RETURN;
    } else if ("rightArrow" == codeStr)
        code = webkit_support::VKEY_RIGHT;
    else if ("downArrow" == codeStr)
        code = webkit_support::VKEY_DOWN;
    else if ("leftArrow" == codeStr)
        code = webkit_support::VKEY_LEFT;
    else if ("upArrow" == codeStr)
        code = webkit_support::VKEY_UP;
    else if ("insert" == codeStr)
        code = webkit_support::VKEY_INSERT;
    else if ("delete" == codeStr)
        code = webkit_support::VKEY_DELETE;
    else if ("pageUp" == codeStr)
        code = webkit_support::VKEY_PRIOR;
    else if ("pageDown" == codeStr)
        code = webkit_support::VKEY_NEXT;
    else if ("home" == codeStr)
        code = webkit_support::VKEY_HOME;
    else if ("end" == codeStr)
        code = webkit_support::VKEY_END;
    else if ("printScreen" == codeStr)
        code = webkit_support::VKEY_SNAPSHOT;
    else if ("menu" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_APPS.
        code = 0x5D;
    else if ("leftControl" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_LCONTROL.
        code = 0xA2;
    else if ("rightControl" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_RCONTROL.
        code = 0xA3;
    else if ("leftShift" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_LSHIFT.
        code = 0xA0;
    else if ("rightShift" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_RSHIFT.
        code = 0xA1;
    else if ("leftAlt" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_LMENU.
        code = 0xA4;
    else if ("rightAlt" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_RMENU.
        code = 0xA5;
    else {
        // Compare the input string with the function-key names defined by the
        // DOM spec (i.e. "F1",...,"F24"). If the input string is a function-key
        // name, set its key code.
        for (int i = 1; i <= 24; ++i) {
            char functionChars[10];
            snprintf(functionChars, 10, "F%d", i);
            string functionKeyName(functionChars);
            if (functionKeyName == codeStr) {
                code = webkit_support::VKEY_F1 + (i - 1);
                break;
            }
        }
        if (!code) {
            WebString webCodeStr = WebString::fromUTF8(codeStr.data(), codeStr.size());
            ASSERT(webCodeStr.length() == 1);
            text = code = webCodeStr.data()[0];
            needsShiftKeyModifier = needsShiftModifier(code);
            if ((code & 0xFF) >= 'a' && (code & 0xFF) <= 'z')
                code -= 'a' - 'A';
            generateChar = true;
        }
    }

    // For one generated keyboard event, we need to generate a keyDown/keyUp
    // pair; refer to EventSender.cpp in Tools/DumpRenderTree/win.
    // On Windows, we might also need to generate a char event to mimic the
    // Windows event flow; on other platforms we create a merged event and test
    // the event flow that that platform provides.
    WebKeyboardEvent eventDown, eventChar, eventUp;
    eventDown.type = WebInputEvent::RawKeyDown;
    eventDown.modifiers = 0;
    eventDown.windowsKeyCode = code;
#if OS(LINUX) && USE(GTK)
    eventDown.nativeKeyCode = webkit_support::NativeKeyCodeForWindowsKeyCode(code, needsShiftKeyModifier);
#endif

    if (generateChar) {
        eventDown.text[0] = text;
        eventDown.unmodifiedText[0] = text;
    }
    eventDown.setKeyIdentifierFromWindowsKeyCode();

    if (arguments.size() >= 2 && (arguments[1].isObject() || arguments[1].isString()))
        eventDown.isSystemKey = applyKeyModifiers(&(arguments[1]), &eventDown);

    if (needsShiftKeyModifier)
        eventDown.modifiers |= WebInputEvent::ShiftKey;

    // See if KeyLocation argument is given.
    if (arguments.size() >= 3 && arguments[2].isNumber()) {
        int location = arguments[2].toInt32();
        if (location == DOMKeyLocationNumpad)
            eventDown.modifiers |= WebInputEvent::IsKeyPad;
    }

    eventChar = eventUp = eventDown;
    eventUp.type = WebInputEvent::KeyUp;
    // EventSender.m forces a layout here, with at least one
    // test (fast/forms/focus-control-to-page.html) relying on this.
    webview()->layout();

    // In the browser, if a keyboard event corresponds to an editor command,
    // the command will be dispatched to the renderer just before dispatching
    // the keyboard event, and then it will be executed in the
    // RenderView::handleCurrentKeyboardEvent() method, which is called from
    // third_party/WebKit/Source/WebKit/chromium/src/EditorClientImpl.cpp.
    // We just simulate the same behavior here.
    string editCommand;
    if (getEditCommand(eventDown, &editCommand))
        m_delegate->setEditCommand(editCommand, "");

    webview()->handleInputEvent(eventDown);

    m_delegate->clearEditCommand();

    if (generateChar) {
        eventChar.type = WebInputEvent::Char;
        eventChar.keyIdentifier[0] = '\0';
        webview()->handleInputEvent(eventChar);
    }

    webview()->handleInputEvent(eventUp);
}

void EventSender::dispatchMessage(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

#if OS(WINDOWS)
    if (arguments.size() == 3) {
        // Grab the message id to see if we need to dispatch it.
        int msg = arguments[0].toInt32();

        // WebKit's version of this function stuffs a MSG struct and uses
        // TranslateMessage and DispatchMessage. We use a WebKeyboardEvent, which
        // doesn't need to receive the DeadChar and SysDeadChar messages.
        if (msg == WM_DEADCHAR || msg == WM_SYSDEADCHAR)
            return;

        webview()->layout();

        unsigned long lparam = static_cast<unsigned long>(arguments[2].toDouble());
        webview()->handleInputEvent(WebInputEventFactory::keyboardEvent(0, msg, arguments[1].toInt32(), lparam));
    } else
        ASSERT_NOT_REACHED();
#endif
}

bool EventSender::needsShiftModifier(int keyCode)
{
    // If code is an uppercase letter, assign a SHIFT key to
    // eventDown.modifier, this logic comes from
    // Tools/DumpRenderTree/win/EventSender.cpp
    return (keyCode & 0xFF) >= 'A' && (keyCode & 0xFF) <= 'Z';
}

void EventSender::leapForward(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    if (arguments.size() < 1 || !arguments[0].isNumber())
        return;

    int milliseconds = arguments[0].toInt32();
    if (isDragMode() && pressedButton == WebMouseEvent::ButtonLeft && !replayingSavedEvents) {
        SavedEvent savedEvent;
        savedEvent.type = SavedEvent::LeapForward;
        savedEvent.milliseconds = milliseconds;
        mouseEventQueue.append(savedEvent);
    } else
        doLeapForward(milliseconds);
}

void EventSender::doLeapForward(int milliseconds)
{
    advanceEventTime(milliseconds);
}

// Apple's port of WebKit zooms by a factor of 1.2 (see
// WebKit/WebView/WebView.mm)
void EventSender::textZoomIn(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(true, webview()->zoomLevel() + 1);
    result->setNull();
}

void EventSender::textZoomOut(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(true, webview()->zoomLevel() - 1);
    result->setNull();
}

void EventSender::zoomPageIn(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(false, webview()->zoomLevel() + 1);
    result->setNull();
}

void EventSender::zoomPageOut(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(false, webview()->zoomLevel() - 1);
    result->setNull();
}

void EventSender::scalePageBy(const CppArgumentList& arguments, CppVariant* result)
{
    if (arguments.size() < 3 || !arguments[0].isNumber() || !arguments[1].isNumber() || !arguments[2].isNumber())
        return;

    float scaleFactor = static_cast<float>(arguments[0].toDouble());
    int x = arguments[1].toInt32();
    int y = arguments[2].toInt32();
    webview()->setPageScaleFactor(scaleFactor, WebPoint(x, y));
    result->setNull();
}

void EventSender::mouseScrollBy(const CppArgumentList& arguments, CppVariant* result)
{
    handleMouseWheel(arguments, result, false);
}

void EventSender::continuousMouseScrollBy(const CppArgumentList& arguments, CppVariant* result)
{
    handleMouseWheel(arguments, result, true);
}

void EventSender::replaySavedEvents()
{
    replayingSavedEvents = true;
    while (!mouseEventQueue.isEmpty()) {
        SavedEvent e = mouseEventQueue.takeFirst();

        switch (e.type) {
        case SavedEvent::MouseMove: {
            WebMouseEvent event;
            initMouseEvent(WebInputEvent::MouseMove, pressedButton, e.pos, &event);
            doMouseMove(event);
            break;
        }
        case SavedEvent::LeapForward:
            doLeapForward(e.milliseconds);
            break;
        case SavedEvent::MouseUp: {
            WebMouseEvent event;
            initMouseEvent(WebInputEvent::MouseUp, e.buttonType, lastMousePos, &event);
            doMouseUp(event);
            break;
        }
        default:
            ASSERT_NOT_REACHED();
        }
    }

    replayingSavedEvents = false;
}

// Because actual context menu is implemented by the browser side, 
// this function does only what LayoutTests are expecting:
// - Many test checks the count of items. So returning non-zero value makes sense.
// - Some test compares the count before and after some action. So changing the count based on flags
//   also makes sense. This function is doing such for some flags.
// - Some test even checks actual string content. So providing it would be also helpful.
//
static Vector<WebString> makeMenuItemStringsFor(WebContextMenuData* contextMenu, TestDelegate* delegate)
{
    // These constants are based on Safari's context menu because tests are made for it.
    static const char* nonEditableMenuStrings[] = { "Back", "Reload Page", "Open in Dashbaord", "<separator>", "View Source", "Save Page As", "Print Page", "Inspect Element", 0 };
    static const char* editableMenuStrings[] = { "Cut", "Copy", "<separator>", "Paste", "Spelling and Grammar", "Substitutions, Transformations", "Font", "Speech", "Paragraph Direction", "<separator>", 0 };

    // This is possible because mouse events are cancelleable.
    if (!contextMenu)
        return Vector<WebString>();

    Vector<WebString> strings;

    if (contextMenu->isEditable) {
        for (const char** item = editableMenuStrings; *item; ++item) 
            strings.append(WebString::fromUTF8(*item));
        Vector<WebString> suggestions;
        delegate->fillSpellingSuggestionList(contextMenu->misspelledWord, &suggestions);
        for (size_t i = 0; i < suggestions.size(); ++i) 
            strings.append(suggestions[i]);
    } else {
        for (const char** item = nonEditableMenuStrings; *item; ++item) 
            strings.append(WebString::fromUTF8(*item));
    }

    return strings;
}

void EventSender::contextClick(const CppArgumentList& arguments, CppVariant* result)
{
    webview()->layout();

    updateClickCountForButton(WebMouseEvent::ButtonRight);

    // Clears last context menu data because we need to know if the context menu be requested 
    // after following mouse events.
    m_delegate->clearContextMenuData();

    // Generate right mouse down and up.
    WebMouseEvent event;
    pressedButton = WebMouseEvent::ButtonRight;
    initMouseEvent(WebInputEvent::MouseDown, WebMouseEvent::ButtonRight, lastMousePos, &event);
    webview()->handleInputEvent(event);

    initMouseEvent(WebInputEvent::MouseUp, WebMouseEvent::ButtonRight, lastMousePos, &event);
    webview()->handleInputEvent(event);

    pressedButton = WebMouseEvent::ButtonNone;

    WebContextMenuData* lastContextMenu = m_delegate->lastContextMenuData();
    result->set(WebBindings::makeStringArray(makeMenuItemStringsFor(lastContextMenu, m_delegate)));
}

class MouseDownTask: public MethodTask<EventSender> {
public:
    MouseDownTask(EventSender* obj, const CppArgumentList& arg)
        : MethodTask<EventSender>(obj), m_arguments(arg) { }
    virtual void runIfValid() { m_object->mouseDown(m_arguments, 0); }

private:
    CppArgumentList m_arguments;
};

class MouseUpTask: public MethodTask<EventSender> {
public:
    MouseUpTask(EventSender* obj, const CppArgumentList& arg)
        : MethodTask<EventSender>(obj), m_arguments(arg) { }
    virtual void runIfValid() { m_object->mouseUp(m_arguments, 0); }

private:
    CppArgumentList m_arguments;
};

void EventSender::scheduleAsynchronousClick(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    postTask(new MouseDownTask(this, arguments));
    postTask(new MouseUpTask(this, arguments));
}

class KeyDownTask : public MethodTask<EventSender> {
public:
    KeyDownTask(EventSender* obj, const CppArgumentList& arg)
        : MethodTask<EventSender>(obj), m_arguments(arg) { }
    virtual void runIfValid() { m_object->keyDown(m_arguments, 0); }

private:
    CppArgumentList m_arguments;
};

void EventSender::scheduleAsynchronousKeyDown(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    postTask(new KeyDownTask(this, arguments));
}

void EventSender::beginDragWithFiles(const CppArgumentList& arguments, CppVariant* result)
{
    currentDragData.initialize();
    Vector<string> files = arguments[0].toStringVector();
    Vector<WebString> absoluteFilenames;
    for (size_t i = 0; i < files.size(); ++i) {
        WebDragData::Item item;
        item.storageType = WebDragData::Item::StorageTypeFilename;
        item.filenameData = webkit_support::GetAbsoluteWebStringFromUTF8Path(files[i]);
        currentDragData.addItem(item);
        absoluteFilenames.append(item.filenameData);
    }
    currentDragData.setFilesystemId(webkit_support::RegisterIsolatedFileSystem(absoluteFilenames));
    currentDragEffectsAllowed = WebKit::WebDragOperationCopy;

    // Provide a drag source.
    webview()->dragTargetDragEnter(currentDragData, lastMousePos, lastMousePos, currentDragEffectsAllowed, 0);

    // dragMode saves events and then replays them later. We don't need/want that.
    dragMode.set(false);

    // Make the rest of eventSender think a drag is in progress.
    pressedButton = WebMouseEvent::ButtonLeft;

    result->setNull();
}

void EventSender::addTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    WebTouchPoint touchPoint;
    touchPoint.state = WebTouchPoint::StatePressed;
    touchPoint.position = WebPoint(arguments[0].toInt32(), arguments[1].toInt32());
    touchPoint.screenPosition = touchPoint.position;

    int lowestId = 0;
    for (size_t i = 0; i < touchPoints.size(); i++) {
        if (touchPoints[i].id == lowestId)
            lowestId++;
    }
    touchPoint.id = lowestId;
    touchPoints.append(touchPoint);
}

void EventSender::clearTouchPoints(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    touchPoints.clear();
}

void EventSender::releaseTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    const unsigned index = arguments[0].toInt32();
    ASSERT(index < touchPoints.size());

    WebTouchPoint* touchPoint = &touchPoints[index];
    touchPoint->state = WebTouchPoint::StateReleased;
}

void EventSender::setTouchModifier(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    int mask = 0;
    const string keyName = arguments[0].toString();
    if (keyName == "shift")
        mask = WebInputEvent::ShiftKey;
    else if (keyName == "alt")
        mask = WebInputEvent::AltKey;
    else if (keyName == "ctrl")
        mask = WebInputEvent::ControlKey;
    else if (keyName == "meta")
        mask = WebInputEvent::MetaKey;

    if (arguments[1].toBoolean())
        touchModifiers |= mask;
    else
        touchModifiers &= ~mask;
}

void EventSender::updateTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    const unsigned index = arguments[0].toInt32();
    ASSERT(index < touchPoints.size());

    WebPoint position(arguments[1].toInt32(), arguments[2].toInt32());
    WebTouchPoint* touchPoint = &touchPoints[index];
    touchPoint->state = WebTouchPoint::StateMoved;
    touchPoint->position = position;
    touchPoint->screenPosition = position;
}

void EventSender::cancelTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    const unsigned index = arguments[0].toInt32();
    ASSERT(index < touchPoints.size());

    WebTouchPoint* touchPoint = &touchPoints[index];
    touchPoint->state = WebTouchPoint::StateCancelled;
}

void EventSender::sendCurrentTouchEvent(const WebInputEvent::Type type)
{
    ASSERT(static_cast<unsigned>(WebTouchEvent::touchesLengthCap) > touchPoints.size());
    webview()->layout();

    WebTouchEvent touchEvent;
    touchEvent.type = type;
    touchEvent.modifiers = touchModifiers;
    touchEvent.timeStampSeconds = getCurrentEventTimeSec();
    touchEvent.touchesLength = touchPoints.size();
    for (unsigned i = 0; i < touchPoints.size(); ++i)
        touchEvent.touches[i] = touchPoints[i];
    webview()->handleInputEvent(touchEvent);

    for (unsigned i = 0; i < touchPoints.size(); ++i) {
        WebTouchPoint* touchPoint = &touchPoints[i];
        if (touchPoint->state == WebTouchPoint::StateReleased) {
            touchPoints.remove(i);
            --i;
        } else
            touchPoint->state = WebTouchPoint::StateStationary;
    }
}

void EventSender::handleMouseWheel(const CppArgumentList& arguments, CppVariant* result, bool continuous)
{
    result->setNull();

    if (arguments.size() < 2 || !arguments[0].isNumber() || !arguments[1].isNumber())
        return;

    // Force a layout here just to make sure every position has been
    // determined before we send events (as well as all the other methods
    // that send an event do).
    webview()->layout();

    int horizontal = arguments[0].toInt32();
    int vertical = arguments[1].toInt32();
    int paged = false;
    int hasPreciseScrollingDeltas = false;

    if (arguments.size() > 2 && arguments[2].isBool())
        paged = arguments[2].toBoolean();

    if (arguments.size() > 3 && arguments[3].isBool())
        hasPreciseScrollingDeltas = arguments[3].toBoolean();

    WebMouseWheelEvent event;
    initMouseEvent(WebInputEvent::MouseWheel, pressedButton, lastMousePos, &event);
    event.wheelTicksX = static_cast<float>(horizontal);
    event.wheelTicksY = static_cast<float>(vertical);
    event.deltaX = event.wheelTicksX;
    event.deltaY = event.wheelTicksY;
    event.scrollByPage = paged;
    event.hasPreciseScrollingDeltas = hasPreciseScrollingDeltas;

    if (continuous) {
        event.wheelTicksX /= scrollbarPixelsPerTick;
        event.wheelTicksY /= scrollbarPixelsPerTick;
    } else {
        event.deltaX *= scrollbarPixelsPerTick;
        event.deltaY *= scrollbarPixelsPerTick;
    }
    webview()->handleInputEvent(event);
}

void EventSender::touchEnd(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchEnd);
}

void EventSender::touchMove(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchMove);
}

void EventSender::touchStart(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchStart);
}

void EventSender::touchCancel(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchCancel);
}

void EventSender::gestureScrollBegin(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureScrollBegin, arguments);
}

void EventSender::gestureScrollEnd(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureScrollEnd, arguments);
}

void EventSender::gestureScrollUpdate(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureScrollUpdate, arguments);
}

void EventSender::gestureTap(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureTap, arguments);
}

void EventSender::gestureTapDown(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureTapDown, arguments);
}

void EventSender::gestureTapCancel(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureTapCancel, arguments);
}

void EventSender::gestureLongPress(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureLongPress, arguments);
}

void EventSender::gestureTwoFingerTap(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureTwoFingerTap, arguments);
}

void EventSender::gestureScrollFirstPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    if (arguments.size() < 2 || !arguments[0].isNumber() || !arguments[1].isNumber())
        return;

    WebPoint point(arguments[0].toInt32(), arguments[1].toInt32());
    m_currentGestureLocation = point;
}

void EventSender::gestureEvent(WebInputEvent::Type type, const CppArgumentList& arguments)
{
    if (arguments.size() < 2 || !arguments[0].isNumber() || !arguments[1].isNumber())
        return;

    WebPoint point(arguments[0].toInt32(), arguments[1].toInt32());

    WebGestureEvent event;
    event.type = type;

    switch (type) {
    case WebInputEvent::GestureScrollUpdate:
        event.data.scrollUpdate.deltaX = static_cast<float>(arguments[0].toDouble());
        event.data.scrollUpdate.deltaY = static_cast<float>(arguments[1].toDouble());
        event.x = m_currentGestureLocation.x;
        event.y = m_currentGestureLocation.y;
        m_currentGestureLocation.x = m_currentGestureLocation.x + event.data.scrollUpdate.deltaX;
        m_currentGestureLocation.y = m_currentGestureLocation.y + event.data.scrollUpdate.deltaY;
        break;

    case WebInputEvent::GestureScrollBegin:
        m_currentGestureLocation = WebPoint(point.x, point.y);
        event.x = m_currentGestureLocation.x;
        event.y = m_currentGestureLocation.y;
        break;
    case WebInputEvent::GestureScrollEnd:
        event.x = m_currentGestureLocation.x;
        event.y = m_currentGestureLocation.y;
        break;
    case WebInputEvent::GestureTap:
        if (arguments.size() >= 3)
            event.data.tap.tapCount = static_cast<float>(arguments[2].toDouble());
        else
          event.data.tap.tapCount = 1;
        event.x = point.x;
        event.y = point.y;
        break;
    case WebInputEvent::GestureTapDown:
        event.x = point.x;
        event.y = point.y;
        if (arguments.size() >= 4) {
            event.data.tapDown.width = static_cast<float>(arguments[2].toDouble());
            event.data.tapDown.height = static_cast<float>(arguments[3].toDouble());
        }
        break;
    case WebInputEvent::GestureTapCancel:
        event.x = point.x;
        event.y = point.y;
        break;
    case WebInputEvent::GestureLongPress:
        event.x = point.x;
        event.y = point.y;
        break;
    case WebInputEvent::GestureTwoFingerTap:
        event.x = point.x;
        event.y = point.y;
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    event.globalX = event.x;
    event.globalY = event.y;
    event.timeStampSeconds = getCurrentEventTimeSec();
    webview()->handleInputEvent(event);
}

void EventSender::gestureFlingCancel(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    if (!arguments.size())
        return;

    WebGestureEvent event;
    event.type = WebInputEvent::GestureFlingCancel;
    event.timeStampSeconds = getCurrentEventTimeSec();
    webview()->handleInputEvent(event);
}

void EventSender::gestureFlingStart(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    if (arguments.size() < 4)
        return;

    for (int i = 0; i < 4; i++)
        if (!arguments[i].isNumber())
            return;

    WebGestureEvent event;
    event.type = WebInputEvent::GestureFlingStart;

    event.x = static_cast<float>(arguments[0].toDouble());
    event.y = static_cast<float>(arguments[1].toDouble());
    event.globalX = event.x;
    event.globalY = event.y;

    event.data.flingStart.velocityX = static_cast<float>(arguments[2].toDouble());
    event.data.flingStart.velocityY = static_cast<float>(arguments[3].toDouble());
    event.timeStampSeconds = getCurrentEventTimeSec();
    webview()->handleInputEvent(event);
}

//
// Unimplemented stubs
//

void EventSender::enableDOMUIEventLogging(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
}

void EventSender::fireKeyboardEventsToElement(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
}

void EventSender::clearKillRing(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
}
