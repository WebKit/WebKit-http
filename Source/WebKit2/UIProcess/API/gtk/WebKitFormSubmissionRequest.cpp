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
#include "WebKitFormSubmissionRequest.h"

#include "WebKitFormSubmissionRequestPrivate.h"
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

G_DEFINE_TYPE(WebKitFormSubmissionRequest, webkit_form_submission_request, G_TYPE_OBJECT)

struct _WebKitFormSubmissionRequestPrivate {
    WKRetainPtr<WKDictionaryRef> wkValues;
    WKRetainPtr<WKFormSubmissionListenerRef> wkListener;
    GRefPtr<GHashTable> values;
    bool handledRequest;
};

static void webkit_form_submission_request_init(WebKitFormSubmissionRequest* request)
{
    WebKitFormSubmissionRequestPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(request, WEBKIT_TYPE_FORM_SUBMISSION_REQUEST, WebKitFormSubmissionRequestPrivate);
    request->priv = priv;
    new (priv) WebKitFormSubmissionRequestPrivate();
}

static void webkitFormSubmissionRequestFinalize(GObject* object)
{
    WebKitFormSubmissionRequest* request = WEBKIT_FORM_SUBMISSION_REQUEST(object);

    // Make sure the request is always handled before finalizing.
    if (!request->priv->handledRequest)
        webkit_form_submission_request_submit(request);

    request->priv->~WebKitFormSubmissionRequestPrivate();
    G_OBJECT_CLASS(webkit_form_submission_request_parent_class)->finalize(object);
}

static void webkit_form_submission_request_class_init(WebKitFormSubmissionRequestClass* requestClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(requestClass);
    objectClass->finalize = webkitFormSubmissionRequestFinalize;
    g_type_class_add_private(requestClass, sizeof(WebKitFormSubmissionRequestPrivate));
}

WebKitFormSubmissionRequest* webkitFormSubmissionRequestCreate(WKDictionaryRef wkValues, WKFormSubmissionListenerRef wkListener)
{
    WebKitFormSubmissionRequest* request = WEBKIT_FORM_SUBMISSION_REQUEST(g_object_new(WEBKIT_TYPE_FORM_SUBMISSION_REQUEST, NULL));
    request->priv->wkValues = wkValues;
    request->priv->wkListener = wkListener;
    return request;
}

/**
 * webkit_form_submission_request_get_text_fields:
 * @request: a #WebKitFormSubmissionRequest
 *
 * Get a #GHashTable with the values of the text fields contained in the form
 * associated to @request.
 *
 * Returns: (transfer none): a #GHashTable with the form text fields, or %NULL if the
 *    form doesn't contain text fields.
 */
GHashTable* webkit_form_submission_request_get_text_fields(WebKitFormSubmissionRequest* request)
{
    g_return_val_if_fail(WEBKIT_IS_FORM_SUBMISSION_REQUEST(request), 0);

    if (request->priv->values)
        return request->priv->values.get();

    if (!WKDictionaryGetSize(request->priv->wkValues.get()))
        return 0;

    request->priv->values = adoptGRef(g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free));

    WKRetainPtr<WKArrayRef> wkKeys(AdoptWK, WKDictionaryCopyKeys(request->priv->wkValues.get()));
    for (size_t i = 0; i < WKArrayGetSize(wkKeys.get()); ++i) {
        WKStringRef wkKey = static_cast<WKStringRef>(WKArrayGetItemAtIndex(wkKeys.get(), i));
        WKStringRef wkValue = static_cast<WKStringRef>(WKDictionaryGetItemForKey(request->priv->wkValues.get(), wkKey));
        g_hash_table_insert(request->priv->values.get(), g_strdup(toImpl(wkKey)->string().utf8().data()), g_strdup(toImpl(wkValue)->string().utf8().data()));
    }

    request->priv->wkValues = 0;

    return request->priv->values.get();
}

/**
 * webkit_form_submission_request_submit:
 * @request: a #WebKitFormSubmissionRequest
 *
 * Continue the form submission.
 */
void webkit_form_submission_request_submit(WebKitFormSubmissionRequest* request)
{
    g_return_if_fail(WEBKIT_IS_FORM_SUBMISSION_REQUEST(request));

    WKFormSubmissionListenerContinue(request->priv->wkListener.get());
    request->priv->handledRequest = true;
}
