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
#include "WebKitWebResource.h"

#include "WebKitMarshal.h"
#include "WebKitURIRequest.h"
#include "WebKitWebResourcePrivate.h"
#include <glib/gi18n-lib.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

enum {
    SENT_REQUEST,
    RECEIVED_DATA,
    FINISHED,
    FAILED,

    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_URI,
    PROP_RESPONSE
};


struct _WebKitWebResourcePrivate {
    WKRetainPtr<WKFrameRef> wkFrame;
    CString uri;
    GRefPtr<WebKitURIResponse> response;
    bool isMainResource;
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(WebKitWebResource, webkit_web_resource, G_TYPE_OBJECT)

static void webkitWebResourceFinalize(GObject* object)
{
    WEBKIT_WEB_RESOURCE(object)->priv->~WebKitWebResourcePrivate();
    G_OBJECT_CLASS(webkit_web_resource_parent_class)->finalize(object);
}

static void webkitWebResourceGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* paramSpec)
{
    WebKitWebResource* resource = WEBKIT_WEB_RESOURCE(object);

    switch (propId) {
    case PROP_URI:
        g_value_set_string(value, webkit_web_resource_get_uri(resource));
        break;
    case PROP_RESPONSE:
        g_value_set_object(value, webkit_web_resource_get_response(resource));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, paramSpec);
    }
}

static void webkit_web_resource_init(WebKitWebResource* resource)
{
    WebKitWebResourcePrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(resource, WEBKIT_TYPE_WEB_RESOURCE, WebKitWebResourcePrivate);
    resource->priv = priv;
    new (priv) WebKitWebResourcePrivate();
}

static void webkit_web_resource_class_init(WebKitWebResourceClass* resourceClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(resourceClass);
    objectClass->get_property = webkitWebResourceGetProperty;
    objectClass->finalize = webkitWebResourceFinalize;

    /**
     * WebKitWebResource:uri:
     *
     * The current active URI of the #WebKitWebResource.
     * See webkit_web_resource_get_uri() for more details.
     */
    g_object_class_install_property(objectClass,
                                    PROP_URI,
                                    g_param_spec_string("uri",
                                                        _("URI"),
                                                        _("The current active URI of the result"),
                                                        0,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebResource:response:
     *
     * The #WebKitURIResponse associated with this resource.
     */
    g_object_class_install_property(objectClass,
                                    PROP_RESPONSE,
                                    g_param_spec_object("response",
                                                        _("Response"),
                                                        _("The response of the resource"),
                                                        WEBKIT_TYPE_URI_RESPONSE,
                                                        WEBKIT_PARAM_READABLE));

    /**
     * WebKitWebResource::sent-request:
     * @resource: the #WebKitWebResource
     * @request: a #WebKitURIRequest
     * @redirected_response: a #WebKitURIResponse, or %NULL
     *
     * This signal is emitted when @request has been sent to the
     * server. In case of a server redirection this signal is
     * emitted again with the @request argument containing the new
     * request sent to the server due to the redirection and the
     * @redirected_response parameter containing the response
     * received by the server for the initial request.
     */
    signals[SENT_REQUEST] =
        g_signal_new("sent-request",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     webkit_marshal_VOID__OBJECT_OBJECT,
                     G_TYPE_NONE, 2,
                     WEBKIT_TYPE_URI_REQUEST,
                     WEBKIT_TYPE_URI_RESPONSE);

    /**
     * WebKitWebResource::received-data:
     * @resource: the #WebKitWebResource
     * @data_length: the length of data received in bytes
     *
     * This signal is emitted after response is received,
     * every time new data has been received. It's
     * useful to know the progress of the resource load operation.
     */
    signals[RECEIVED_DATA] =
        g_signal_new("received-data",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     webkit_marshal_VOID__UINT64,
                     G_TYPE_NONE, 1,
                     G_TYPE_UINT64);

    /**
     * WebKitWebResource::finished:
     * @resource: the #WebKitWebResource
     *
     * This signal is emitted when the resource load finishes successfully
     * or due to an error. In case of errors #WebKitWebResource::failed signal
     * is emitted before this one.
     */
    signals[FINISHED] =
        g_signal_new("finished",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /**
     * WebKitWebResource::failed:
     * @resource: the #WebKitWebResource
     * @error: the #GError that was triggered
     *
     * This signal is emitted when an error occurs during the resource
     * load operation.
     */
    signals[FAILED] =
        g_signal_new("failed",
                     G_TYPE_FROM_CLASS(objectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__POINTER,
                     G_TYPE_NONE, 1,
                     G_TYPE_POINTER);

    g_type_class_add_private(resourceClass, sizeof(WebKitWebResourcePrivate));
}

static void webkitWebResourceUpdateURI(WebKitWebResource* resource, const CString& requestURI)
{
    if (resource->priv->uri == requestURI)
        return;

    resource->priv->uri = requestURI;
    g_object_notify(G_OBJECT(resource), "uri");
}

WebKitWebResource* webkitWebResourceCreate(WKFrameRef wkFrame, WebKitURIRequest* request, bool isMainResource)
{
    ASSERT(wkFrame);
    WebKitWebResource* resource = WEBKIT_WEB_RESOURCE(g_object_new(WEBKIT_TYPE_WEB_RESOURCE, NULL));
    resource->priv->wkFrame = wkFrame;
    resource->priv->uri = webkit_uri_request_get_uri(request);
    resource->priv->isMainResource = isMainResource;
    return resource;
}

void webkitWebResourceSentRequest(WebKitWebResource* resource, WebKitURIRequest* request, WebKitURIResponse* redirectResponse)
{
    webkitWebResourceUpdateURI(resource, webkit_uri_request_get_uri(request));
    g_signal_emit(resource, signals[SENT_REQUEST], 0, request, redirectResponse);
}

void webkitWebResourceSetResponse(WebKitWebResource* resource, WebKitURIResponse* response)
{
    resource->priv->response = response;
    g_object_notify(G_OBJECT(resource), "response");
}

void webkitWebResourceNotifyProgress(WebKitWebResource* resource, guint64 bytesReceived)
{
    g_signal_emit(resource, signals[RECEIVED_DATA], 0, bytesReceived);
}

void webkitWebResourceFinished(WebKitWebResource* resource)
{
    g_signal_emit(resource, signals[FINISHED], 0, NULL);
}

void webkitWebResourceFailed(WebKitWebResource* resource, GError* error)
{
    g_signal_emit(resource, signals[FAILED], 0, error);
    g_signal_emit(resource, signals[FINISHED], 0, NULL);
}

WKFrameRef webkitWebResourceGetFrame(WebKitWebResource* resource)
{
    return resource->priv->wkFrame.get();
}

/**
 * webkit_web_resource_get_uri:
 * @resource: a #WebKitWebResource
 *
 * Returns the current active URI of @web_view. The active URI might change during
 * a load operation:
 *
 * <orderedlist>
 * <listitem><para>
 *   When the resource load starts, the active URI is the requested URI
 * </para></listitem>
 * <listitem><para>
 *   When the initial request is sent to the server, #WebKitWebResource::sent-request
 *   signal is emitted without a redirected response, the active URI is the URI of
 *   the request sent to the server.
 * </para></listitem>
 * <listitem><para>
 *   In case of a server redirection, #WebKitWebResource::sent-request signal
 *   is emitted again with a redirected response, the active URI is the URI the request
 *   was redirected to.
 * </para></listitem>
 * <listitem><para>
 *   When the response is received from the server, the active URI is the final
 *   one and it will not change again.
 * </para></listitem>
 * </orderedlist>
 *
 * You can monitor the active URI by connecting to the notify::uri
 * signal of @resource.
 *
 * Returns: the current active URI of @resource
 */
const char* webkit_web_resource_get_uri(WebKitWebResource* resource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(resource), 0);

    return resource->priv->uri.data();
}

/**
 * webkit_web_resource_get_response:
 * @resource: a #WebKitWebResource
 *
 * Retrieves the #WebKitURIResponse of the resource load operation.
 * This method returns %NULL if called before the response
 * is received from the server. You can connect to notify::response
 * signal to be notified when the response is received.
 *
 * Returns: (transfer none): the #WebKitURIResponse, or %NULL if
 *     the response hasn't been received yet.
 */
WebKitURIResponse* webkit_web_resource_get_response(WebKitWebResource* resource)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(resource), 0);

    return resource->priv->response.get();
}

struct ResourceGetDataAsyncData {
    WKDataRef wkData;
    GRefPtr<GCancellable> cancellable;
};
WEBKIT_DEFINE_ASYNC_DATA_STRUCT(ResourceGetDataAsyncData)

static void resourceDataCallback(WKDataRef wkData, WKErrorRef, void* context)
{
    GRefPtr<GSimpleAsyncResult> result = adoptGRef(G_SIMPLE_ASYNC_RESULT(context));
    ResourceGetDataAsyncData* data = static_cast<ResourceGetDataAsyncData*>(g_simple_async_result_get_op_res_gpointer(result.get()));
    GError* error = 0;
    if (g_cancellable_set_error_if_cancelled(data->cancellable.get(), &error))
        g_simple_async_result_take_error(result.get(), error);
    else
        data->wkData = wkData;
    g_simple_async_result_complete(result.get());
}

/**
 * webkit_web_resource_get_data:
 * @resource: a #WebKitWebResource
 * @cancellable: (allow-none): a #GCancellable or %NULL to ignore
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously get the raw data for @resource.
 *
 * When the operation is finished, @callback will be called. You can then call
 * webkit_web_resource_get_data_finish() to get the result of the operation.
 */
void webkit_web_resource_get_data(WebKitWebResource* resource, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    g_return_if_fail(WEBKIT_IS_WEB_RESOURCE(resource));

    GSimpleAsyncResult* result = g_simple_async_result_new(G_OBJECT(resource), callback, userData,
                                                           reinterpret_cast<gpointer>(webkit_web_resource_get_data));
    ResourceGetDataAsyncData* data = createResourceGetDataAsyncData();
    data->cancellable = cancellable;
    g_simple_async_result_set_op_res_gpointer(result, data, reinterpret_cast<GDestroyNotify>(destroyResourceGetDataAsyncData));
    if (resource->priv->isMainResource)
        WKFrameGetMainResourceData(resource->priv->wkFrame.get(), resourceDataCallback, result);
    else {
        WKRetainPtr<WKURLRef> url(AdoptWK, WKURLCreateWithUTF8CString(resource->priv->uri.data()));
        WKFrameGetResourceData(resource->priv->wkFrame.get(), url.get(), resourceDataCallback, result);
    }
}

/**
 * webkit_web_resource_get_data_finish:
 * @resource: a #WebKitWebResource
 * @result: a #GAsyncResult
 * @length: (out): return location for the length of the resource data
 * @error: return location for error or %NULL to ignore
 *
 * Finish an asynchronous operation started with webkit_web_resource_get_data().
 *
 * Returns: (transfer full): a string with the data of @resource, or %NULL in case
 *    of error. if @length is not %NULL, the size of the data will be assigned to it.
 */
guchar* webkit_web_resource_get_data_finish(WebKitWebResource* resource, GAsyncResult* result, gsize* length, GError** error)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_RESOURCE(resource), 0);
    g_return_val_if_fail(G_IS_ASYNC_RESULT(result), 0);

    GSimpleAsyncResult* simple = G_SIMPLE_ASYNC_RESULT(result);
    g_warn_if_fail(g_simple_async_result_get_source_tag(simple) == webkit_web_resource_get_data);

    if (g_simple_async_result_propagate_error(simple, error))
        return 0;

    ResourceGetDataAsyncData* data = static_cast<ResourceGetDataAsyncData*>(g_simple_async_result_get_op_res_gpointer(simple));
    if (length)
        *length = WKDataGetSize(data->wkData);
    return static_cast<guchar*>(g_memdup(WKDataGetBytes(data->wkData), WKDataGetSize(data->wkData)));
}
