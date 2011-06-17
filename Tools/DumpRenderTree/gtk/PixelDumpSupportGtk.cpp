/*
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2010 Igalia S.L.
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

#include "DumpRenderTree.h"
#include "GtkVersioning.h"
#include "PixelDumpSupportCairo.h"
#include "WebCoreSupport/DumpRenderTreeSupportGtk.h"
#include <webkit/webkit.h>

PassRefPtr<BitmapContext> createBitmapContextFromWebView(bool, bool, bool, bool drawSelectionRect)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    GtkWidget* viewContainer = gtk_widget_get_parent(GTK_WIDGET(view));
    gint width, height;
#ifdef GTK_API_VERSION_2
    GdkPixmap* pixmap = gtk_widget_get_snapshot(viewContainer, 0);
    gdk_pixmap_get_size(pixmap, &width, &height);
#else
    width = gtk_widget_get_allocated_width(viewContainer);
    height = gtk_widget_get_allocated_height(viewContainer);
#endif

    cairo_surface_t* imageSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* context = cairo_create(imageSurface);

#ifdef GTK_API_VERSION_2
    gdk_cairo_set_source_pixmap(context, pixmap, 0, 0);
    cairo_paint(context);
    g_object_unref(pixmap);
#else
    gtk_widget_draw(viewContainer, context);
#endif

    if (drawSelectionRect) {
        cairo_rectangle_int_t rectangle;
        DumpRenderTreeSupportGtk::rectangleForSelection(mainFrame, &rectangle);

        cairo_set_line_width(context, 1.0);
        cairo_rectangle(context, rectangle.x, rectangle.y, rectangle.width, rectangle.height);
        cairo_set_source_rgba(context, 1.0, 0.0, 0.0, 1.0);
        cairo_stroke(context);
    }

    return BitmapContext::createByAdoptingBitmapAndContext(0, context);
}
