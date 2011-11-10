/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef EventSenderProxy_h
#define EventSenderProxy_h

#if PLATFORM(QT)
#include <QEvent>
#include <QTouchEvent>
#elif PLATFORM(GTK)
#include <gdk/gdk.h>
#include <wtf/Vector.h>
#endif

namespace WTR {

class TestController;

#if PLATFORM(GTK)
struct WTREventQueueItem;
#endif

class EventSenderProxy {
public:
    EventSenderProxy(TestController*);

    void mouseDown(unsigned button, WKEventModifiers);
    void mouseUp(unsigned button, WKEventModifiers);
    void mouseMoveTo(double x, double y);
    void mouseScrollBy(int x, int y);

    void leapForward(int milliseconds);

    void keyDown(WKStringRef key, WKEventModifiers, unsigned location);

#if ENABLE(TOUCH_EVENTS)
    // Touch events.
    void addTouchPoint(int x, int y);
    void updateTouchPoint(int index, int x, int y);
    void setTouchModifier(WKEventModifiers, bool enable);
    void touchStart();
    void touchMove();
    void touchEnd();
    void clearTouchPoints();
    void releaseTouchPoint(int index);
#endif

private:
    TestController* m_testController;

    double currentEventTime() { return m_time; }
    void updateClickCountForButton(int button);

#if PLATFORM(QT) || PLATFORM(GTK)
    void replaySavedEvents();
#endif

#if PLATFORM(QT)
#if ENABLE(TOUCH_EVENTS)
    void sendTouchEvent(QEvent::Type);
#endif
    void sendOrQueueEvent(QEvent*);
#elif PLATFORM(GTK)
    void sendOrQueueEvent(GdkEvent*);
    GdkEvent* createMouseButtonEvent(GdkEventType, unsigned button, WKEventModifiers);
#endif

    double m_time;
    WKPoint m_position;
    bool m_leftMouseButtonDown;
    int m_clickCount;
    double m_clickTime;
    WKPoint m_clickPosition;
    WKEventMouseButton m_clickButton;
#if PLATFORM(MAC)
    int eventNumber;
#elif PLATFORM(GTK)
    Vector<WTREventQueueItem> m_eventQueue;
    unsigned m_mouseButtonCurrentlyDown;
#elif PLATFORM(QT)
    Qt::MouseButtons m_mouseButtons;

#if ENABLE(TOUCH_EVENTS)
    QList<QTouchEvent::TouchPoint> m_touchPoints;
    Qt::KeyboardModifiers m_touchModifiers;
    bool m_touchActive;
#endif
#endif
};

} // namespace WTR

#endif // EventSenderProxy_h
