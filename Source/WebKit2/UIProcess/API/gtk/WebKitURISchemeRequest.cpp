/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebKitURISchemeRequest.h"

#include "WebKitURISchemeRequestPrivate.h"
#include "WebKitWebContextPrivate.h"
#include <WebCore/GOwnPtrSoup.h>
#include <libsoup/soup.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

static const unsigned int gReadBufferSize = 8192;

G_DEFINE_TYPE(WebKitURISchemeRequest, webkit_uri_scheme_request, G_TYPE_OBJECT)

struct _WebKitURISchemeRequestPrivate {
    WebKitWebContext* webContext;
    WKRetainPtr<WKSoupRequestManagerRef> wkRequestManager;
    uint64_t requestID;
    CString uri;
    GOwnPtr<SoupURI> soupURI;

    GRefPtr<GInputStream> stream;
    uint64_t streamLength;
    GRefPtr<GCancellable> cancellable;
    char readBuffer[gReadBufferSize];
    uint64_t bytesRead;
    CString mimeType;
};

static void webkit_uri_scheme_request_init(WebKitURISchemeRequest* request)
{
    WebKitURISchemeRequestPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(request, WEBKIT_TYPE_URI_SCHEME_REQUEST, WebKitURISchemeRequestPrivate);
    request->priv = priv;
    new (priv) WebKitURISchemeRequestPrivate();
}

static void webkitURISchemeRequestFinalize(GObject* object)
{
    WEBKIT_URI_SCHEME_REQUEST(object)->priv->~WebKitURISchemeRequestPrivate();
    G_OBJECT_CLASS(webkit_uri_scheme_request_parent_class)->finalize(object);
}

static void webkit_uri_scheme_request_class_init(WebKitURISchemeRequestClass* requestClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(requestClass);
    objectClass->finalize = webkitURISchemeRequestFinalize;
    g_type_class_add_private(requestClass, sizeof(WebKitURISchemeRequestPrivate));
}

WebKitURISchemeRequest* webkitURISchemeRequestCreate(WebKitWebContext* webContext, WKSoupRequestManagerRef wkRequestManager, WKURLRef wkURL, uint64_t requestID)
{
    WebKitURISchemeRequest* request = WEBKIT_URI_SCHEME_REQUEST(g_object_new(WEBKIT_TYPE_URI_SCHEME_REQUEST, NULL));
    request->priv->webContext = webContext;
    request->priv->wkRequestManager = wkRequestManager;
    request->priv->uri = toImpl(wkURL)->string().utf8();
    request->priv->requestID = requestID;
    return request;
}

uint64_t webkitURISchemeRequestGetID(WebKitURISchemeRequest* request)
{
    return request->priv->requestID;
}

void webkitURISchemeRequestCancel(WebKitURISchemeRequest* request)
{
    if (request->priv->cancellable.get())
        g_cancellable_cancel(request->priv->cancellable.get());
}

/**
 * webkit_uri_scheme_request_get_scheme:
 * @request: a #WebKitURISchemeRequest
 *
 * Get the URI scheme of @request
 *
 * Returns: the URI scheme of @request
 */
const char* webkit_uri_scheme_request_get_scheme(WebKitURISchemeRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request), 0);

    if (!request->priv->soupURI)
        request->priv->soupURI.set(soup_uri_new(request->priv->uri.data()));
    return request->priv->soupURI->scheme;
}

/**
 * webkit_uri_scheme_request_get_uri:
 * @request: a #WebKitURISchemeRequest
 *
 * Get the URI of @request
 *
 * Returns: the full URI of @request
 */
const char* webkit_uri_scheme_request_get_uri(WebKitURISchemeRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request), 0);

    return request->priv->uri.data();
}

/**
 * webkit_uri_scheme_request_get_path:
 * @request: a #WebKitURISchemeRequest
 *
 * Get the URI path of @request
 *
 * Returns: the URI path of @request
 */
const char* webkit_uri_scheme_request_get_path(WebKitURISchemeRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request), 0);

    if (!request->priv->soupURI)
        request->priv->soupURI.set(soup_uri_new(request->priv->uri.data()));
    return request->priv->soupURI->path;
}

static void webkitURISchemeRequestReadCallback(GInputStream* inputStream, GAsyncResult* result, WebKitURISchemeRequest* schemeRequest)
{
    GRefPtr<WebKitURISchemeRequest> request = adoptGRef(schemeRequest);
    gssize bytesRead = g_input_stream_read_finish(inputStream, result, 0);
    // FIXME: notify the WebProcess that we failed to read from the user stream.
    if (bytesRead == -1) {
        webkitWebContextDidFinishURIRequest(request->priv->webContext, request->priv->requestID);
        return;
    }

    WebKitURISchemeRequestPrivate* priv = request->priv;
    WKRetainPtr<WKDataRef> wkData(AdoptWK, WKDataCreate(bytesRead ? reinterpret_cast<const unsigned char*>(priv->readBuffer) : 0, bytesRead));
    if (!priv->bytesRead) {
        // First chunk read. In case of empty reply an empty WKDataRef is sent to the WebProcess.
        WKRetainPtr<WKStringRef> wkMimeType = !priv->mimeType.isNull() ? adoptWK(WKStringCreateWithUTF8CString(priv->mimeType.data())) : 0;
        WKSoupRequestManagerDidHandleURIRequest(priv->wkRequestManager.get(), wkData.get(), priv->streamLength, wkMimeType.get(), priv->requestID);
    } else if (bytesRead || (!bytesRead && !priv->streamLength)) {
        // Subsequent chunk read. We only send an empty WKDataRef to the WebProcess when stream length is unknown.
        WKSoupRequestManagerDidReceiveURIRequestData(priv->wkRequestManager.get(), wkData.get(), priv->requestID);
    }

    if (!bytesRead) {
        webkitWebContextDidFinishURIRequest(request->priv->webContext, request->priv->requestID);
        return;
    }

    priv->bytesRead += bytesRead;
    g_input_stream_read_async(inputStream, priv->readBuffer, gReadBufferSize, G_PRIORITY_DEFAULT, priv->cancellable.get(),
                              reinterpret_cast<GAsyncReadyCallback>(webkitURISchemeRequestReadCallback), g_object_ref(request.get()));
}

/**
 * webkit_uri_scheme_request_finish:
 * @request: a #WebKitURISchemeRequest
 * @stream: a #GInputStream to read the contents of the request
 * @stream_length: the length of the stream or -1 if not known
 * @mime_type: (allow-none): the content type of the stream or %NULL if not known
 *
 * Finish a #WebKitURISchemeRequest by setting the contents of the request and its mime type.
 */
void webkit_uri_scheme_request_finish(WebKitURISchemeRequest* request, GInputStream* inputStream, gint64 streamLength, const gchar* mimeType)
{
    g_return_if_fail(WEBKIT_IS_URI_SCHEME_REQUEST(request));
    g_return_if_fail(G_IS_INPUT_STREAM(inputStream));
    g_return_if_fail(streamLength == -1 || streamLength >= 0);

    request->priv->stream = inputStream;
    // We use -1 in the API for consistency with soup when the content length is not known, but 0 internally.
    request->priv->streamLength = streamLength == -1 ? 0 : streamLength;
    request->priv->cancellable = adoptGRef(g_cancellable_new());
    request->priv->bytesRead = 0;
    request->priv->mimeType = mimeType;
    g_input_stream_read_async(inputStream, request->priv->readBuffer, gReadBufferSize, G_PRIORITY_DEFAULT, request->priv->cancellable.get(),
                              reinterpret_cast<GAsyncReadyCallback>(webkitURISchemeRequestReadCallback), g_object_ref(request));
}
