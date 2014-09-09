/*
 * Copyright (C) 2004, 2005, 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd.  All rights reserved.
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

#ifndef Widget_h
#define Widget_h

#if PLATFORM(IOS)
#ifndef NSView
#define NSView WAKView
#endif
#endif

#include "IntRect.h"
#include "PlatformScreen.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

#if PLATFORM(COCOA)
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(COCOA)
OBJC_CLASS NSView;
OBJC_CLASS NSWindow;
typedef NSView *PlatformWidget;
#endif

#if PLATFORM(WIN)
typedef struct HWND__* HWND;
typedef HWND PlatformWidget;
#endif

#if PLATFORM(GTK)
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkContainer GtkContainer;
typedef GtkWidget* PlatformWidget;
#endif

#if PLATFORM(EFL)
typedef Evas_Object* PlatformWidget;
#endif

typedef PlatformWidget PlatformPageClient;

namespace WebCore {

class Cursor;
class Event;
class Font;
class GraphicsContext;
class PlatformMouseEvent;
class ScrollView;

enum WidgetNotification { WillPaintFlattened, DidPaintFlattened };

// The Widget class serves as a base class for three kinds of objects:
// (1) Scrollable areas (ScrollView)
// (2) Scrollbars (Scrollbar)
// (3) Plugins (PluginView)
//
// A widget may or may not be backed by a platform-specific object (e.g., HWND on Windows, NSView on Mac, QWidget on Qt).
//
// Widgets are connected in a hierarchy, with the restriction that plugins and scrollbars are always leaves of the
// tree.  Only ScrollViews can have children (and therefore the Widget class has no concept of children).
//
// The rules right now for which widgets get platform-specific objects are as follows:
// ScrollView - Mac
// Scrollbar - Mac, Gtk
// Plugin - Mac, Windows (windowed only), Qt (windowed only, widget is an HWND on windows), Gtk (windowed only)
//
class Widget : public RefCounted<Widget> {
public:
    explicit Widget(PlatformWidget = 0);
    virtual ~Widget();

    PlatformWidget platformWidget() const;
    void setPlatformWidget(PlatformWidget);

    int x() const { return frameRect().x(); }
    int y() const { return frameRect().y(); }
    int width() const { return frameRect().width(); }
    int height() const { return frameRect().height(); }
    IntSize size() const { return frameRect().size(); }
    IntPoint location() const { return frameRect().location(); }

    virtual void setFrameRect(const IntRect&);
    IntRect frameRect() const;
    IntRect boundsRect() const { return IntRect(0, 0, width(),  height()); }

    void resize(int w, int h) { setFrameRect(IntRect(x(), y(), w, h)); }
    void resize(const IntSize& s) { setFrameRect(IntRect(location(), s)); }
    void move(int x, int y) { setFrameRect(IntRect(x, y, width(), height())); }
    void move(const IntPoint& p) { setFrameRect(IntRect(p, size())); }

    virtual void paint(GraphicsContext*, const IntRect&);
    void invalidate() { invalidateRect(boundsRect()); }
    virtual void invalidateRect(const IntRect&) = 0;

    virtual void setFocus(bool);

    void setCursor(const Cursor&);

    virtual void show();
    virtual void hide();
    bool isSelfVisible() const { return m_selfVisible; } // Whether or not we have been explicitly marked as visible or not.
    bool isParentVisible() const { return m_parentVisible; } // Whether or not our parent is visible.
    bool isVisible() const { return m_selfVisible && m_parentVisible; } // Whether or not we are actually visible.
    virtual void setParentVisible(bool visible) { m_parentVisible = visible; }
    void setSelfVisible(bool v) { m_selfVisible = v; }

    void setIsSelected(bool);

    virtual bool isFrameView() const { return false; }
    virtual bool isPluginView() const { return false; }
    // FIXME: The Mac plug-in code should inherit from PluginView. When this happens PluginViewBase and PluginView can become one class.
    virtual bool isPluginViewBase() const { return false; }
    virtual bool isScrollbar() const { return false; }
    virtual bool isScrollView() const { return false; }

    void removeFromParent();
    virtual void setParent(ScrollView* view);
    ScrollView* parent() const { return m_parent; }
    ScrollView* root() const;

    virtual void handleEvent(Event*) { }

    virtual void notifyWidget(WidgetNotification) { }

    IntRect convertToRootView(const IntRect&) const;
    IntRect convertFromRootView(const IntRect&) const;

    IntPoint convertToRootView(const IntPoint&) const;
    IntPoint convertFromRootView(const IntPoint&) const;

    // It is important for cross-platform code to realize that Mac has flipped coordinates.  Therefore any code
    // that tries to convert the location of a rect using the point-based convertFromContainingWindow will end
    // up with an inaccurate rect.  Always make sure to use the rect-based convertFromContainingWindow method
    // when converting window rects.
    IntRect convertToContainingWindow(const IntRect&) const;
    IntRect convertFromContainingWindow(const IntRect&) const;

    IntPoint convertToContainingWindow(const IntPoint&) const;
    IntPoint convertFromContainingWindow(const IntPoint&) const;

    virtual void frameRectsChanged() { }

    // Notifies this widget that its clip rect changed.
    virtual void clipRectChanged() { }

    // Whether transforms affect the frame rect. FIXME: We get rid of this and have
    // the frame rects be the same no matter what transforms are applied.
    virtual bool transformsAffectFrameRect() { return true; }

#if PLATFORM(COCOA)
    NSView* getOuterView() const;

    void removeFromSuperview();
#endif
#if PLATFORM(IOS)
    void addToSuperview(NSView*);
#endif

    // Virtual methods to convert points to/from the containing ScrollView
    virtual IntRect convertToContainingView(const IntRect&) const;
    virtual IntRect convertFromContainingView(const IntRect&) const;
    virtual IntPoint convertToContainingView(const IntPoint&) const;
    virtual IntPoint convertFromContainingView(const IntPoint&) const;

private:
    void init(PlatformWidget); // Must be called by all Widget constructors to initialize cross-platform data.

    void releasePlatformWidget();
    void retainPlatformWidget();

    // These methods are used to convert from the root widget to the containing window,
    // which has behavior that may differ between platforms (e.g. Mac uses flipped window coordinates).
    static IntRect convertFromRootToContainingWindow(const Widget* rootWidget, const IntRect&);
    static IntRect convertFromContainingWindowToRoot(const Widget* rootWidget, const IntRect&);

    static IntPoint convertFromRootToContainingWindow(const Widget* rootWidget, const IntPoint&);
    static IntPoint convertFromContainingWindowToRoot(const Widget* rootWidget, const IntPoint&);

private:
    ScrollView* m_parent;
#if !PLATFORM(COCOA)
    PlatformWidget m_widget;
#else
    RetainPtr<NSView> m_widget;
#endif
    bool m_selfVisible;
    bool m_parentVisible;

    IntRect m_frame; // Not used when a native widget exists.
};

#define WIDGET_TYPE_CASTS(ToValueTypeName, predicate) \
    TYPE_CASTS_BASE(ToValueTypeName, Widget, object, object->predicate, object.predicate)

#if !PLATFORM(COCOA)

inline PlatformWidget Widget::platformWidget() const
{
    return m_widget;
}

inline void Widget::setPlatformWidget(PlatformWidget widget)
{
    if (widget != m_widget) {
        releasePlatformWidget();
        m_widget = widget;
        retainPlatformWidget();
    }
}

#endif

#if !PLATFORM(GTK)

inline void Widget::releasePlatformWidget()
{
}

inline void Widget::retainPlatformWidget()
{
}

#endif

} // namespace WebCore

#endif // Widget_h
