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
#include "WebSoupRequestManager.h"

#include "DataReference.h"
#include "MessageID.h"
#include "WebErrors.h"
#include "WebKitSoupRequestGeneric.h"
#include "WebKitSoupRequestInputStream.h"
#include "WebProcess.h"
#include "WebSoupRequestManagerProxyMessages.h"
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceRequest.h>
#include <libsoup/soup-requester.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

namespace WebKit {

static uint64_t generateSoupRequestID()
{
    static uint64_t uniqueSoupRequestID = 1;
    return uniqueSoupRequestID++;
}

struct WebSoupRequestAsyncData {
    WebSoupRequestAsyncData(GSimpleAsyncResult* result, WebKitSoupRequestGeneric* requestGeneric, GCancellable* cancellable)
        : result(result)
        , request(requestGeneric)
        , cancellable(cancellable)
    {
        // If the struct contains a null request, it is because the request failed.
        g_object_add_weak_pointer(G_OBJECT(request), reinterpret_cast<void**>(&request));
    }

    ~WebSoupRequestAsyncData()
    {
        if (request)
            g_object_remove_weak_pointer(G_OBJECT(request), reinterpret_cast<void**>(&request));
    }

    bool requestFailed()
    {
        return g_cancellable_is_cancelled(cancellable.get()) || !request;
    }

    GRefPtr<GSimpleAsyncResult> releaseResult()
    {
        GSimpleAsyncResult* returnValue = result;
        result = 0;
        return adoptGRef(returnValue);
    }

    GSimpleAsyncResult* result;
    WebKitSoupRequestGeneric* request;
    GRefPtr<GCancellable> cancellable;
    GRefPtr<GInputStream> stream;
};

WebSoupRequestManager::WebSoupRequestManager(WebProcess* process)
    : m_process(process)
    , m_schemes(adoptGRef(g_ptr_array_new_with_free_func(g_free)))
{
}

WebSoupRequestManager::~WebSoupRequestManager()
{
}

void WebSoupRequestManager::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebSoupRequestManagerMessage(connection, messageID, arguments);
}

void WebSoupRequestManager::registerURIScheme(const String& scheme)
{
    if (m_schemes->len)
        g_ptr_array_remove_index_fast(m_schemes.get(), m_schemes->len - 1);
    g_ptr_array_add(m_schemes.get(), g_strdup(scheme.utf8().data()));
    g_ptr_array_add(m_schemes.get(), 0);

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    GRefPtr<SoupRequester> requester = SOUP_REQUESTER(soup_session_get_feature(session, SOUP_TYPE_REQUESTER));
    if (requester)
        soup_session_feature_remove_feature(SOUP_SESSION_FEATURE(requester.get()), WEBKIT_TYPE_SOUP_REQUEST_GENERIC);
    else {
        requester = adoptGRef(soup_requester_new());
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(requester.get()));
    }
    SoupRequestClass* genericRequestClass = static_cast<SoupRequestClass*>(g_type_class_ref(WEBKIT_TYPE_SOUP_REQUEST_GENERIC));
    genericRequestClass->schemes = const_cast<const char**>(reinterpret_cast<char**>(m_schemes->pdata));
    soup_session_feature_add_feature(SOUP_SESSION_FEATURE(requester.get()), WEBKIT_TYPE_SOUP_REQUEST_GENERIC);
}

void WebSoupRequestManager::didHandleURIRequest(const CoreIPC::DataReference& requestData, uint64_t contentLength, const String& mimeType, uint64_t requestID)
{
    WebSoupRequestAsyncData* data = m_requestMap.get(requestID);
    ASSERT(data);
    GRefPtr<GSimpleAsyncResult> result = data->releaseResult();
    ASSERT(result.get());

    GRefPtr<WebKitSoupRequestGeneric> request = adoptGRef(WEBKIT_SOUP_REQUEST_GENERIC(g_async_result_get_source_object(G_ASYNC_RESULT(result.get()))));
    webkitSoupRequestGenericSetContentLength(request.get(), contentLength ? contentLength : -1);
    webkitSoupRequestGenericSetContentType(request.get(), !mimeType.isEmpty() ? mimeType.utf8().data() : 0);

    GInputStream* dataStream;
    if (!requestData.size()) {
        // Empty reply, just create and empty GMemoryInputStream.
        dataStream = g_memory_input_stream_new();
        m_requestMap.remove(requestID);
    } else if (requestData.size() == contentLength) {
        // We don't expect more data, so we can just create a GMemoryInputStream with all the data.
        dataStream = g_memory_input_stream_new_from_data(g_memdup(requestData.data(), requestData.size()), contentLength, g_free);
        m_requestMap.remove(requestID);
    } else {
        // We expect more data chunks from the UI process.
        dataStream = webkitSoupRequestInputStreamNew(contentLength);
        data->stream = dataStream;
        webkitSoupRequestInputStreamAddData(WEBKIT_SOUP_REQUEST_INPUT_STREAM(dataStream), requestData.data(), requestData.size());
    }
    g_simple_async_result_set_op_res_gpointer(result.get(), dataStream, g_object_unref);
    g_simple_async_result_complete(result.get());
}

void WebSoupRequestManager::didReceiveURIRequestData(const CoreIPC::DataReference& requestData, uint64_t requestID)
{
    WebSoupRequestAsyncData* data = m_requestMap.get(requestID);
    // The data might have been removed from the request map if a previous chunk failed
    // and a new message was sent by the UI process before being notified about the failure.
    if (!data)
        return;
    ASSERT(data->stream.get());

    if (data->requestFailed()) {
        // ResourceRequest failed or it was cancelled. It doesn't matter here the error or if it was cancelled,
        // because that's already handled by the resource handle client, we just want to notify the UI process
        // to stop reading data from the user input stream. If UI process already sent all the data we simply
        // finish silently.
        if (!webkitSoupRequestInputStreamFinished(WEBKIT_SOUP_REQUEST_INPUT_STREAM(data->stream.get())))
            m_process->connection()->send(Messages::WebSoupRequestManagerProxy::DidFailToLoadURIRequest(requestID), 0);
        m_requestMap.remove(requestID);

        return;
    }

    webkitSoupRequestInputStreamAddData(WEBKIT_SOUP_REQUEST_INPUT_STREAM(data->stream.get()), requestData.data(), requestData.size());
    if (webkitSoupRequestInputStreamFinished(WEBKIT_SOUP_REQUEST_INPUT_STREAM(data->stream.get())))
        m_requestMap.remove(requestID);
}

void WebSoupRequestManager::send(GSimpleAsyncResult* result, GCancellable* cancellable)
{
    GRefPtr<WebKitSoupRequestGeneric> request = adoptGRef(WEBKIT_SOUP_REQUEST_GENERIC(g_async_result_get_source_object(G_ASYNC_RESULT(result))));
    SoupURI* uri = soup_request_get_uri(SOUP_REQUEST(request.get()));
    GOwnPtr<char> uriString(soup_uri_to_string(uri, FALSE));

    uint64_t requestID = generateSoupRequestID();
    m_requestMap.set(requestID, adoptPtr(new WebSoupRequestAsyncData(result, request.get(), cancellable)));
    m_process->connection()->send(Messages::WebSoupRequestManagerProxy::DidReceiveURIRequest(String::fromUTF8(uriString.get()), requestID), 0);
}

GInputStream* WebSoupRequestManager::finish(GSimpleAsyncResult* result)
{
    return G_INPUT_STREAM(g_object_ref(g_simple_async_result_get_op_res_gpointer(result)));
}

} // namespace WebKit
