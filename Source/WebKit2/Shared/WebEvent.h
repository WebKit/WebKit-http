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

#ifndef WebEvent_h
#define WebEvent_h

// FIXME: We should probably move to makeing the WebCore/PlatformFooEvents trivial classes so that
// we can use them as the event type.

#include <WebCore/FloatSize.h>
#include <WebCore/IntPoint.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

class WebEvent {
public:
    enum Type {
        NoType = -1,
        
        // WebMouseEvent
        MouseDown,
        MouseUp,
        MouseMove,

        // WebWheelEvent
        Wheel,

        // WebKeyboardEvent
        KeyDown,
        KeyUp,
        RawKeyDown,
        Char,

#if ENABLE(GESTURE_EVENTS)
        // WebGestureEvent
        GestureScrollBegin,
        GestureScrollEnd,
#endif

#if ENABLE(TOUCH_EVENTS)
        // WebTouchEvent
        TouchStart,
        TouchMove,
        TouchEnd,
        TouchCancel,
#endif
    };

    enum Modifiers {
        ShiftKey    = 1 << 0,
        ControlKey  = 1 << 1,
        AltKey      = 1 << 2,
        MetaKey     = 1 << 3,
        CapsLockKey = 1 << 4,
    };

    Type type() const { return static_cast<Type>(m_type); }

    bool shiftKey() const { return m_modifiers & ShiftKey; }
    bool controlKey() const { return m_modifiers & ControlKey; }
    bool altKey() const { return m_modifiers & AltKey; }
    bool metaKey() const { return m_modifiers & MetaKey; }
    bool capsLockKey() const { return m_modifiers & CapsLockKey; }

    Modifiers modifiers() const { return static_cast<Modifiers>(m_modifiers); }

    double timestamp() const { return m_timestamp; }

protected:
    WebEvent();

    WebEvent(Type, Modifiers, double timestamp);

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, WebEvent&);

private:
    uint32_t m_type; // Type
    uint32_t m_modifiers; // Modifiers
    double m_timestamp;
};

// FIXME: Move this class to its own header file.
class WebMouseEvent : public WebEvent {
public:
    enum Button {
        NoButton = -1,
        LeftButton,
        MiddleButton,
        RightButton
    };

    WebMouseEvent();

    WebMouseEvent(Type, Button, const WebCore::IntPoint& position, const WebCore::IntPoint& globalPosition, float deltaX, float deltaY, float deltaZ, int clickCount, Modifiers, double timestamp);
#if PLATFORM(WIN)
    WebMouseEvent(Type, Button, const WebCore::IntPoint& position, const WebCore::IntPoint& globalPosition, float deltaX, float deltaY, float deltaZ, int clickCount, Modifiers, double timestamp, bool didActivateWebView);
#endif
    
    WebMouseEvent(const WebMouseEvent&, float pageScaleFactor);

    Button button() const { return static_cast<Button>(m_button); }
    const WebCore::IntPoint& position() const { return m_position; }
    const WebCore::IntPoint& globalPosition() const { return m_globalPosition; }
    float deltaX() const { return m_deltaX; }
    float deltaY() const { return m_deltaY; }
    float deltaZ() const { return m_deltaZ; }
    int32_t clickCount() const { return m_clickCount; }
#if PLATFORM(WIN)
    bool didActivateWebView() const { return m_didActivateWebView; }
#endif

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, WebMouseEvent&);

private:
    static bool isMouseEventType(Type);

    uint32_t m_button;
    WebCore::IntPoint m_position;
    WebCore::IntPoint m_globalPosition;
    float m_deltaX;
    float m_deltaY;
    float m_deltaZ;
    int32_t m_clickCount;
#if PLATFORM(WIN)
    bool m_didActivateWebView;
#endif
};

// FIXME: Move this class to its own header file.
class WebWheelEvent : public WebEvent {
public:
    enum Granularity {
        ScrollByPageWheelEvent,
        ScrollByPixelWheelEvent
    };

#if PLATFORM(MAC)
    enum Phase {
        PhaseNone        = 0,
        PhaseBegan       = 1 << 1,
        PhaseStationary  = 1 << 2,
        PhaseChanged     = 1 << 3,
        PhaseEnded       = 1 << 4,
        PhaseCancelled   = 1 << 5,
    };
#endif

    WebWheelEvent() { }

    WebWheelEvent(Type, const WebCore::IntPoint& position, const WebCore::IntPoint& globalPosition, const WebCore::FloatSize& delta, const WebCore::FloatSize& wheelTicks, Granularity, Modifiers, double timestamp);
#if PLATFORM(MAC)
    WebWheelEvent(Type, const WebCore::IntPoint& position, const WebCore::IntPoint& globalPosition, const WebCore::FloatSize& delta, const WebCore::FloatSize& wheelTicks, Granularity, Phase, Phase momentumPhase, bool hasPreciseScrollingDeltas, Modifiers, double timestamp, bool directionInvertedFromDevice);
#endif

    const WebCore::IntPoint position() const { return m_position; }
    const WebCore::IntPoint globalPosition() const { return m_globalPosition; }
    const WebCore::FloatSize delta() const { return m_delta; }
    const WebCore::FloatSize wheelTicks() const { return m_wheelTicks; }
    Granularity granularity() const { return static_cast<Granularity>(m_granularity); }
    bool directionInvertedFromDevice() const { return m_directionInvertedFromDevice; }
#if PLATFORM(MAC)
    Phase phase() const { return static_cast<Phase>(m_phase); }
    Phase momentumPhase() const { return static_cast<Phase>(m_momentumPhase); }
    bool hasPreciseScrollingDeltas() const { return m_hasPreciseScrollingDeltas; }
#endif

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, WebWheelEvent&);

private:
    static bool isWheelEventType(Type);

    WebCore::IntPoint m_position;
    WebCore::IntPoint m_globalPosition;
    WebCore::FloatSize m_delta;
    WebCore::FloatSize m_wheelTicks;
    uint32_t m_granularity; // Granularity
    bool m_directionInvertedFromDevice;
#if PLATFORM(MAC)
    uint32_t m_phase; // Phase
    uint32_t m_momentumPhase; // Phase
    bool m_hasPreciseScrollingDeltas;
#endif
};

// FIXME: Move this class to its own header file.
class WebKeyboardEvent : public WebEvent {
public:
    WebKeyboardEvent() { }

    WebKeyboardEvent(Type, const String& text, const String& unmodifiedText, const String& keyIdentifier, int windowsVirtualKeyCode, int nativeVirtualKeyCode, int macCharCode, bool isAutoRepeat, bool isKeypad, bool isSystemKey, Modifiers, double timestamp);

    const String& text() const { return m_text; }
    const String& unmodifiedText() const { return m_unmodifiedText; }
    const String& keyIdentifier() const { return m_keyIdentifier; }
    int32_t windowsVirtualKeyCode() const { return m_windowsVirtualKeyCode; }
    int32_t nativeVirtualKeyCode() const { return m_nativeVirtualKeyCode; }
    int32_t macCharCode() const { return m_macCharCode; }
    bool isAutoRepeat() const { return m_isAutoRepeat; }
    bool isKeypad() const { return m_isKeypad; }
    bool isSystemKey() const { return m_isSystemKey; }

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, WebKeyboardEvent&);

    static bool isKeyboardEventType(Type);

private:
    String m_text;
    String m_unmodifiedText;
    String m_keyIdentifier;
    int32_t m_windowsVirtualKeyCode;
    int32_t m_nativeVirtualKeyCode;
    int32_t m_macCharCode;
    bool m_isAutoRepeat;
    bool m_isKeypad;
    bool m_isSystemKey;
};


#if ENABLE(GESTURE_EVENTS)
// FIXME: Move this class to its own header file.
class WebGestureEvent : public WebEvent {
public:
    WebGestureEvent() { }
    WebGestureEvent(Type, const WebCore::IntPoint& position, const WebCore::IntPoint& globalPosition, Modifiers, double timestamp);

    const WebCore::IntPoint position() const { return m_position; }
    const WebCore::IntPoint globalPosition() const { return m_globalPosition; }

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, WebGestureEvent&);

private:
    static bool isGestureEventType(Type);

    WebCore::IntPoint m_position;
    WebCore::IntPoint m_globalPosition;
};
#endif // ENABLE(GESTURE_EVENTS)


#if ENABLE(TOUCH_EVENTS)
// FIXME: Move this class to its own header file.
// FIXME: Having "Platform" in the name makes it sound like this event is platform-specific or low-
// level in some way. That doesn't seem to be the case.
class WebPlatformTouchPoint {
public:
    enum TouchPointState {
        TouchReleased,
        TouchPressed,
        TouchMoved,
        TouchStationary,
        TouchCancelled
    };

    WebPlatformTouchPoint() { }

    WebPlatformTouchPoint(uint32_t id, TouchPointState, const WebCore::IntPoint& screenPosition, const WebCore::IntPoint& position);

    uint32_t id() const { return m_id; }
    TouchPointState state() const { return static_cast<TouchPointState>(m_state); }

    const WebCore::IntPoint& screenPosition() const { return m_screenPosition; }
    const WebCore::IntPoint& position() const { return m_position; }
          
    void setState(TouchPointState state) { m_state = state; }

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, WebPlatformTouchPoint&);

private:
    uint32_t m_id;
    uint32_t m_state;
    WebCore::IntPoint m_screenPosition;
    WebCore::IntPoint m_position;

};

// FIXME: Move this class to its own header file.
class WebTouchEvent : public WebEvent {
public:
    WebTouchEvent() { }
 
    // FIXME: It would be nice not to have to copy the Vector here.
    WebTouchEvent(Type, Vector<WebPlatformTouchPoint>, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, Modifiers, double timestamp);

    const Vector<WebPlatformTouchPoint>& touchPoints() const { return m_touchPoints; }

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, WebTouchEvent&);
  
private:
    static bool isTouchEventType(Type);

    Vector<WebPlatformTouchPoint> m_touchPoints;
    bool m_ctrlKey;
    bool m_altKey;
    bool m_shiftKey;
    bool m_metaKey;
};

#endif // ENABLE(TOUCH_EVENTS)

} // namespace WebKit

#endif // WebEvent_h
