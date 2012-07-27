/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "config.h"

#include "UnitTestUtils/EWK2UnitTestBase.h"
#include "UnitTestUtils/EWK2UnitTestEnvironment.h"
#include "UnitTestUtils/EWK2UnitTestServer.h"
#include <EWebKit2.h>
#include <Ecore.h>
#include <gtest/gtest.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static void onLoadFinishedForRedirection(void* userData, Evas_Object*, void*)
{
    int* countLoadFinished = static_cast<int*>(userData);
    (*countLoadFinished)--;
}

TEST_F(EWK2UnitTestBase, ewk_view_uri_get)
{
    loadUrlSync(environment->defaultTestPageUrl());
    EXPECT_STREQ(ewk_view_uri_get(webView()), environment->defaultTestPageUrl());

    int countLoadFinished = 2;
    evas_object_smart_callback_add(webView(), "load,finished", onLoadFinishedForRedirection, &countLoadFinished);
    ewk_view_uri_set(webView(), environment->urlForResource("redirect_uri_to_default.html").data());
    while (countLoadFinished)
        ecore_main_loop_iterate();
    evas_object_smart_callback_del(webView(), "load,finished", onLoadFinishedForRedirection);
    EXPECT_STREQ(ewk_view_uri_get(webView()), environment->defaultTestPageUrl());
}

TEST_F(EWK2UnitTestBase, ewk_view_device_pixel_ratio)
{
    loadUrlSync(environment->defaultTestPageUrl());

    // Default pixel ratio is 1.0
    ASSERT_FLOAT_EQ(ewk_view_device_pixel_ratio_get(webView()), 1);

    ASSERT_TRUE(ewk_view_device_pixel_ratio_set(webView(), 1.2));
    ASSERT_FLOAT_EQ(ewk_view_device_pixel_ratio_get(webView()), 1.2);

    ASSERT_TRUE(ewk_view_device_pixel_ratio_set(webView(), 1));
    ASSERT_FLOAT_EQ(ewk_view_device_pixel_ratio_get(webView()), 1);
}

TEST_F(EWK2UnitTestBase, ewk_view_html_string_load)
{
    ewk_view_html_string_load(webView(), "<html><head><title>Foo</title></head><body>Bar</body></html>", 0, 0);
    waitUntilTitleChangedTo("Foo");
    ASSERT_STREQ(ewk_view_title_get(webView()), "Foo");
    ewk_view_html_string_load(webView(), "<html><head><title>Bar</title></head><body>Foo</body></html>", 0, 0);
    waitUntilTitleChangedTo("Bar");
    ASSERT_STREQ(ewk_view_title_get(webView()), "Bar");
}

static void serverCallbackNavigation(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);

    Eina_Strbuf* body = eina_strbuf_new();
    eina_strbuf_append_printf(body, "<html><title>%s</title><body>%s</body></html>", path + 1, path + 1);
    const size_t bodyLength = eina_strbuf_length_get(body);
    soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, eina_strbuf_string_steal(body), bodyLength);
    eina_strbuf_free(body);

    soup_message_body_complete(message->response_body);
}

TEST_F(EWK2UnitTestBase, ewk_view_navigation)
{
    OwnPtr<EWK2UnitTestServer> httpServer = adoptPtr(new EWK2UnitTestServer);
    httpServer->run(serverCallbackNavigation);

    // Visit Page1
    loadUrlSync(httpServer->getURIForPath("/Page1").data());
    ASSERT_STREQ(ewk_view_title_get(webView()), "Page1");
    ASSERT_FALSE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));

    // Visit Page2
    loadUrlSync(httpServer->getURIForPath("/Page2").data());
    ASSERT_STREQ(ewk_view_title_get(webView()), "Page2");
    ASSERT_TRUE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));

    // Go back to Page1
    ewk_view_back(webView());
    waitUntilTitleChangedTo("Page1");
    ASSERT_STREQ(ewk_view_title_get(webView()), "Page1");
    ASSERT_FALSE(ewk_view_back_possible(webView()));
    ASSERT_TRUE(ewk_view_forward_possible(webView()));

    // Go forward to Page2
    ewk_view_forward(webView());
    waitUntilTitleChangedTo("Page2");
    ASSERT_STREQ(ewk_view_title_get(webView()), "Page2");
    ASSERT_TRUE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_view_setting_encoding_custom)
{
    ASSERT_FALSE(ewk_view_setting_encoding_custom_get(webView()));
    ASSERT_TRUE(ewk_view_setting_encoding_custom_set(webView(), "UTF-8"));
    ASSERT_STREQ(ewk_view_setting_encoding_custom_get(webView()), "UTF-8");
    // Set the default charset.
    ASSERT_TRUE(ewk_view_setting_encoding_custom_set(webView(), 0));
    ASSERT_FALSE(ewk_view_setting_encoding_custom_get(webView()));
}

static void onFormAboutToBeSubmitted(void* userData, Evas_Object*, void* eventInfo)
{
    Ewk_Form_Submission_Request* request = static_cast<Ewk_Form_Submission_Request*>(eventInfo);
    bool* handled = static_cast<bool*>(userData);

    ASSERT_TRUE(request);

    Eina_List* fieldNames = ewk_form_submission_request_field_names_get(request);
    ASSERT_TRUE(fieldNames);
    ASSERT_EQ(eina_list_count(fieldNames), 3);
    void* data;
    EINA_LIST_FREE(fieldNames, data)
        eina_stringshare_del(static_cast<char*>(data));

    const char* value1 = ewk_form_submission_request_field_value_get(request, "text1");
    ASSERT_STREQ(value1, "value1");
    eina_stringshare_del(value1);
    const char* value2 = ewk_form_submission_request_field_value_get(request, "text2");
    ASSERT_STREQ(value2, "value2");
    eina_stringshare_del(value2);
    const char* password = ewk_form_submission_request_field_value_get(request, "password");
    ASSERT_STREQ(password, "secret");
    eina_stringshare_del(password);

    *handled = true;
}

TEST_F(EWK2UnitTestBase, ewk_view_form_submission_request)
{
    const char* formHTML =
        "<html><head><script type='text/javascript'>function submitForm() { document.getElementById('myform').submit(); }</script></head>"
        "<body onload='submitForm()'>"
        " <form id='myform' action='#'>"
        "  <input type='text' name='text1' value='value1'>"
        "  <input type='text' name='text2' value='value2'>"
        "  <input type='password' name='password' value='secret'>"
        "  <textarea cols='5' rows='5' name='textarea'>Text</textarea>"
        "  <input type='hidden' name='hidden1' value='hidden1'>"
        "  <input type='submit' value='Submit'>"
        " </form>"
        "</body></html>";

    ewk_view_html_string_load(webView(), formHTML, "file:///", 0);
    bool handled = false;
    evas_object_smart_callback_add(webView(), "form,submission,request", onFormAboutToBeSubmitted, &handled);
    while (!handled)
        ecore_main_loop_iterate();
    ASSERT_TRUE(handled);
    evas_object_smart_callback_del(webView(), "form,submission,request", onFormAboutToBeSubmitted);
}
