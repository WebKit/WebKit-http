/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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
#include "WebKitWebViewBase.h"

#include "DrawingAreaProxyImpl.h"
#include "GOwnPtrGtk.h"
#include "GtkClickCounter.h"
#include "GtkVersioning.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "NotImplemented.h"
#include "PageClientImpl.h"
#include "RefPtrCairo.h"
#include "WebContext.h"
#include "WebEventFactory.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebPageProxy.h"
#include <WebCore/Region.h>
#include <WebKit2/WKContext.h>
#include <wtf/text/CString.h>

using namespace WebKit;
using namespace WebCore;

struct _WebKitWebViewBasePrivate {
    OwnPtr<PageClientImpl> pageClient;
    RefPtr<WebPageProxy> pageProxy;
    gboolean isPageActive;
    gboolean shouldForwardNextKeyEvent;
    GtkIMContext* imContext;
    GtkClickCounter clickCounter;
    CString tooltipText;
};

G_DEFINE_TYPE(WebKitWebViewBase, webkit_web_view_base, GTK_TYPE_CONTAINER)

static void webkitWebViewBaseRealize(GtkWidget* widget)
{
    gtk_widget_set_realized(widget, TRUE);

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    GdkWindowAttr attributes;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK
        | GDK_EXPOSURE_MASK
        | GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK
        | GDK_POINTER_MOTION_MASK
        | GDK_KEY_PRESS_MASK
        | GDK_KEY_RELEASE_MASK
        | GDK_BUTTON_MOTION_MASK
        | GDK_BUTTON1_MOTION_MASK
        | GDK_BUTTON2_MOTION_MASK
        | GDK_BUTTON3_MOTION_MASK;

    gint attributesMask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    GdkWindow* window = gdk_window_new(gtk_widget_get_parent_window(widget), &attributes, attributesMask);
    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, widget);

    gtk_style_context_set_background(gtk_widget_get_style_context(widget), window);

    WebKitWebViewBase* webView = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webView->priv;
    gtk_im_context_set_client_window(priv->imContext, window);
}

static void webkitWebViewBaseContainerAdd(GtkContainer* container, GtkWidget* widget)
{
    gtk_widget_set_parent(widget, GTK_WIDGET(container));
}

static void webkitWebViewBaseFinalize(GObject* gobject)
{
    WebKitWebViewBase* webkitWebViewBase = WEBKIT_WEB_VIEW_BASE(gobject);
    WebKitWebViewBasePrivate* priv = webkitWebViewBase->priv;

    if (priv->imContext) {
        g_object_unref(priv->imContext);
        priv->imContext = 0;
    }

    priv->pageProxy->close();

    delete priv;
    webkitWebViewBase->priv = 0;

    G_OBJECT_CLASS(webkit_web_view_base_parent_class)->finalize(gobject);
}

static void webkit_web_view_base_init(WebKitWebViewBase* webkitWebViewBase)
{
    WebKitWebViewBasePrivate* priv = new WebKitWebViewBasePrivate();
    webkitWebViewBase->priv = priv;

    priv->isPageActive = TRUE;
    priv->shouldForwardNextKeyEvent = FALSE;

    gtk_widget_set_double_buffered(GTK_WIDGET(webkitWebViewBase), FALSE);
    gtk_widget_set_can_focus(GTK_WIDGET(webkitWebViewBase), TRUE);
    priv->imContext = gtk_im_multicontext_new();

    priv->pageClient = PageClientImpl::create(GTK_WIDGET(webkitWebViewBase));
}

static gboolean webkitWebViewBaseDraw(GtkWidget* widget, cairo_t* cr)
{
    DrawingAreaProxy* drawingArea = WEBKIT_WEB_VIEW_BASE(widget)->priv->pageProxy->drawingArea();
    if (!drawingArea)
        return FALSE;

    GdkRectangle clipRect;
    if (!gdk_cairo_get_clip_rectangle(cr, &clipRect))
        return FALSE;

    WebCore::Region unpaintedRegion; // This is simply unused.
    static_cast<DrawingAreaProxyImpl*>(drawingArea)->paint(cr, clipRect, unpaintedRegion);

    return FALSE;
}

static void webkitWebViewBaseSizeAllocate(GtkWidget* widget, GtkAllocation* allocation)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (!priv->pageProxy->drawingArea())
        return;

    GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->size_allocate(widget, allocation);
    priv->pageProxy->drawingArea()->setSize(IntSize(allocation->width, allocation->height), IntSize());
}

static gboolean webkitWebViewBaseFocusInEvent(GtkWidget* widget, GdkEventFocus* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
    if (gtk_widget_is_toplevel(toplevel) && gtk_window_has_toplevel_focus(GTK_WINDOW(toplevel))) {
        gtk_im_context_focus_in(priv->imContext);
        if (!priv->isPageActive) {
            priv->isPageActive = TRUE;
            priv->pageProxy->viewStateDidChange(WebPageProxy::ViewWindowIsActive);
        }
    }

    return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->focus_in_event(widget, event);
}

static gboolean webkitWebViewBaseFocusOutEvent(GtkWidget* widget, GdkEventFocus* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    priv->isPageActive = FALSE;
    priv->pageProxy->viewStateDidChange(WebPageProxy::ViewWindowIsActive);
    if (priv->imContext)
        gtk_im_context_focus_out(priv->imContext);

    return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->focus_out_event(widget, event);
}

static gboolean webkitWebViewBaseKeyPressEvent(GtkWidget* widget, GdkEventKey* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    // Since WebProcess key event handling is not synchronous, handle the event in two passes.
    // When WebProcess processes the input event, it will call PageClientImpl::doneWithKeyEvent 
    // with event handled status which determines whether to pass the input event to parent or not 
    // using gtk_main_do_event().
    if (priv->shouldForwardNextKeyEvent) {
        priv->shouldForwardNextKeyEvent = FALSE;
        return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->key_press_event(widget, event);
    }
    priv->pageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(reinterpret_cast<GdkEvent*>(event)));
    return TRUE;
}

static gboolean webkitWebViewBaseKeyReleaseEvent(GtkWidget* widget, GdkEventKey* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    if (gtk_im_context_filter_keypress(priv->imContext, event))
        return TRUE;

    if (priv->shouldForwardNextKeyEvent) {
        priv->shouldForwardNextKeyEvent = FALSE;
        return GTK_WIDGET_CLASS(webkit_web_view_base_parent_class)->key_release_event(widget, event);
    }
    priv->pageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(reinterpret_cast<GdkEvent*>(event)));
    return TRUE;
}

static gboolean webkitWebViewBaseButtonPressEvent(GtkWidget* widget, GdkEventButton* buttonEvent)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    gtk_widget_grab_focus(widget);

    if (!priv->clickCounter.shouldProcessButtonEvent(buttonEvent))
        return TRUE;
    priv->pageProxy->handleMouseEvent(NativeWebMouseEvent(reinterpret_cast<GdkEvent*>(buttonEvent),
                                                     priv->clickCounter.clickCountForGdkButtonEvent(widget, buttonEvent)));
    return FALSE;
}

static gboolean webkitWebViewBaseButtonReleaseEvent(GtkWidget* widget, GdkEventButton* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    gtk_widget_grab_focus(widget);
    priv->pageProxy->handleMouseEvent(NativeWebMouseEvent(reinterpret_cast<GdkEvent*>(event), 0 /* currentClickCount */));

    return FALSE;
}

static gboolean webkitWebViewBaseScrollEvent(GtkWidget* widget, GdkEventScroll* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    priv->pageProxy->handleWheelEvent(NativeWebWheelEvent(reinterpret_cast<GdkEvent*>(event)));

    return FALSE;
}

static gboolean webkitWebViewBaseMotionNotifyEvent(GtkWidget* widget, GdkEventMotion* event)
{
    WebKitWebViewBase* webViewBase = WEBKIT_WEB_VIEW_BASE(widget);
    WebKitWebViewBasePrivate* priv = webViewBase->priv;

    priv->pageProxy->handleMouseEvent(NativeWebMouseEvent(reinterpret_cast<GdkEvent*>(event), 0 /* currentClickCount */));

    return FALSE;
}

static gboolean webkitWebViewBaseQueryTooltip(GtkWidget* widget, gint x, gint y, gboolean keyboardMode, GtkTooltip* tooltip)
{
    WebKitWebViewBasePrivate* priv = WEBKIT_WEB_VIEW_BASE(widget)->priv;

    if (keyboardMode) {
        // TODO: https://bugs.webkit.org/show_bug.cgi?id=61732.
        notImplemented();
        return FALSE;
    }

    if (priv->tooltipText.length() <= 0)
        return FALSE;

    // TODO: set the tip area when WKPageMouseDidMoveOverElementCallback
    // receives a hit test result.
    gtk_tooltip_set_text(tooltip, priv->tooltipText.data());
    return TRUE;
}

static void webkit_web_view_base_class_init(WebKitWebViewBaseClass* webkitWebViewBaseClass)
{
    GtkWidgetClass* widgetClass = GTK_WIDGET_CLASS(webkitWebViewBaseClass);
    widgetClass->realize = webkitWebViewBaseRealize;
    widgetClass->draw = webkitWebViewBaseDraw;
    widgetClass->size_allocate = webkitWebViewBaseSizeAllocate;
    widgetClass->focus_in_event = webkitWebViewBaseFocusInEvent;
    widgetClass->focus_out_event = webkitWebViewBaseFocusOutEvent;
    widgetClass->key_press_event = webkitWebViewBaseKeyPressEvent;
    widgetClass->key_release_event = webkitWebViewBaseKeyReleaseEvent;
    widgetClass->button_press_event = webkitWebViewBaseButtonPressEvent;
    widgetClass->button_release_event = webkitWebViewBaseButtonReleaseEvent;
    widgetClass->scroll_event = webkitWebViewBaseScrollEvent;
    widgetClass->motion_notify_event = webkitWebViewBaseMotionNotifyEvent;
    widgetClass->query_tooltip = webkitWebViewBaseQueryTooltip;

    GObjectClass* gobjectClass = G_OBJECT_CLASS(webkitWebViewBaseClass);
    gobjectClass->finalize = webkitWebViewBaseFinalize;

    GtkContainerClass* containerClass = GTK_CONTAINER_CLASS(webkitWebViewBaseClass);
    containerClass->add = webkitWebViewBaseContainerAdd;
}

WebKitWebViewBase* webkitWebViewBaseCreate(WebContext* context, WebPageGroup* pageGroup)
{
    WebKitWebViewBase* webkitWebViewBase = WEBKIT_WEB_VIEW_BASE(g_object_new(WEBKIT_TYPE_WEB_VIEW_BASE, NULL));
    webkitWebViewBaseCreateWebPage(webkitWebViewBase, toAPI(context), toAPI(pageGroup));
    return webkitWebViewBase;
}

GtkIMContext* webkitWebViewBaseGetIMContext(WebKitWebViewBase* webkitWebViewBase)
{
    return webkitWebViewBase->priv->imContext;
}

WebPageProxy* webkitWebViewBaseGetPage(WebKitWebViewBase* webkitWebViewBase)
{
    return webkitWebViewBase->priv->pageProxy.get();
}

void webkitWebViewBaseCreateWebPage(WebKitWebViewBase* webkitWebViewBase, WKContextRef context, WKPageGroupRef pageGroup)
{
    WebKitWebViewBasePrivate* priv = webkitWebViewBase->priv;

    priv->pageProxy = toImpl(context)->createWebPage(priv->pageClient.get(), toImpl(pageGroup));
    priv->pageProxy->initializeWebPage();
}

void webkitWebViewBaseSetTooltipText(WebKitWebViewBase* webViewBase, const char* tooltip)
{
    WebKitWebViewBasePrivate* priv = webViewBase->priv;
    if (tooltip && tooltip[0] != '\0') {
        priv->tooltipText = tooltip;
        gtk_widget_set_has_tooltip(GTK_WIDGET(webViewBase), TRUE);
    } else {
        priv->tooltipText = "";
        gtk_widget_set_has_tooltip(GTK_WIDGET(webViewBase), FALSE);
    }

    gtk_widget_trigger_tooltip_query(GTK_WIDGET(webViewBase));
}

void webkitWebViewBaseForwardNextKeyEvent(WebKitWebViewBase* webkitWebViewBase)
{
    webkitWebViewBase->priv->shouldForwardNextKeyEvent = TRUE;
}
