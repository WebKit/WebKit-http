/*
 * Copyright (C) 2011 Igalia S.L.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebViewTest.h"

WebViewTest::WebViewTest()
    : m_webView(WEBKIT_WEB_VIEW(g_object_ref_sink(webkit_web_view_new())))
    , m_mainLoop(g_main_loop_new(0, TRUE))
{
    assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_webView));
}

WebViewTest::~WebViewTest()
{
    g_object_unref(m_webView);
    g_main_loop_unref(m_mainLoop);
}

static gboolean testLoadTimeoutFinishLoop(GMainLoop* loop)
{
    g_main_loop_quit(loop);
    return FALSE;
}

void WebViewTest::loadURI(const char* uri)
{
    m_activeURI = uri;
    webkit_web_view_load_uri(m_webView, uri);
}

void WebViewTest::loadHtml(const char* html, const char* baseURI)
{
    m_activeURI = "about:blank";
    webkit_web_view_load_html(m_webView, html, baseURI);
}

void WebViewTest::loadPlainText(const char* plainText)
{
    m_activeURI = "about:blank";
    webkit_web_view_load_plain_text(m_webView, plainText);
}

void WebViewTest::loadAlternateHTML(const char* html, const char* baseURI, const char* unreachableURI)
{
    m_activeURI = "about:blank";
    webkit_web_view_load_alternate_html(m_webView, html, baseURI, unreachableURI);
}

void WebViewTest::loadRequest(WebKitURIRequest* request)
{
    m_activeURI = webkit_uri_request_get_uri(request);
    webkit_web_view_load_request(m_webView, request);
}

void WebViewTest::goBack()
{
    if (webkit_web_view_can_go_back(m_webView)) {
        WebKitBackForwardList* list = webkit_web_view_get_back_forward_list(m_webView);
        WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(list, -1);
        m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    }

    // Call go_back even when can_go_back returns FALSE to check nothing happens.
    webkit_web_view_go_back(m_webView);
}

void WebViewTest::goForward()
{
    if (webkit_web_view_can_go_forward(m_webView)) {
        WebKitBackForwardList* list = webkit_web_view_get_back_forward_list(m_webView);
        WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(list, 1);
        m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    }

    // Call go_forward even when can_go_forward returns FALSE to check nothing happens.
    webkit_web_view_go_forward(m_webView);
}

void WebViewTest::goToBackForwardListItem(WebKitBackForwardListItem* item)
{
    m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    webkit_web_view_go_to_back_forward_list_item(m_webView, item);
}

void WebViewTest::wait(double seconds)
{
    g_timeout_add_seconds(seconds, reinterpret_cast<GSourceFunc>(testLoadTimeoutFinishLoop), m_mainLoop);
    g_main_loop_run(m_mainLoop);
}
