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
#include "UnitTestUtils/EWK2UnitTestServer.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;
bool fullScreenCallbackCalled;

TEST_F(EWK2UnitTestBase, ewk_view_type_check)
{
    ASSERT_FALSE(ewk_view_context_get(0));

    Evas_Object* rectangle = evas_object_rectangle_add(canvas());
    ASSERT_FALSE(ewk_view_url_set(rectangle, 0));
}

static void onLoadFinishedForRedirection(void* userData, Evas_Object*, void*)
{
    int* countLoadFinished = static_cast<int*>(userData);
    (*countLoadFinished)--;
}

TEST_F(EWK2UnitTestBase, ewk_view_url_get)
{
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    EXPECT_STREQ(environment->defaultTestPageUrl(), ewk_view_url_get(webView()));

    int countLoadFinished = 2;
    evas_object_smart_callback_add(webView(), "load,finished", onLoadFinishedForRedirection, &countLoadFinished);
    ewk_view_url_set(webView(), environment->urlForResource("redirect_url_to_default.html").data());
    while (countLoadFinished)
        ecore_main_loop_iterate();
    evas_object_smart_callback_del(webView(), "load,finished", onLoadFinishedForRedirection);
    EXPECT_STREQ(environment->defaultTestPageUrl(), ewk_view_url_get(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_view_device_pixel_ratio)
{
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));

    // Default pixel ratio is 1.0
    ASSERT_FLOAT_EQ(1, ewk_view_device_pixel_ratio_get(webView()));

    ASSERT_TRUE(ewk_view_device_pixel_ratio_set(webView(), 1.2));
    ASSERT_FLOAT_EQ(1.2, ewk_view_device_pixel_ratio_get(webView()));

    ASSERT_TRUE(ewk_view_device_pixel_ratio_set(webView(), 1));
    ASSERT_FLOAT_EQ(1, ewk_view_device_pixel_ratio_get(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_view_html_string_load)
{
    ewk_view_html_string_load(webView(), "<html><head><title>Foo</title></head><body>Bar</body></html>", 0, 0);
    ASSERT_TRUE(waitUntilTitleChangedTo("Foo"));
    ASSERT_STREQ("Foo", ewk_view_title_get(webView()));
    ewk_view_html_string_load(webView(), "<html><head><title>Bar</title></head><body>Foo</body></html>", 0, 0);
    ASSERT_TRUE(waitUntilTitleChangedTo("Bar"));
    ASSERT_STREQ("Bar", ewk_view_title_get(webView()));
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
    ASSERT_TRUE(loadUrlSync(httpServer->getURLForPath("/Page1").data()));
    ASSERT_STREQ("Page1", ewk_view_title_get(webView()));
    ASSERT_FALSE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));

    // Visit Page2
    ASSERT_TRUE(loadUrlSync(httpServer->getURLForPath("/Page2").data()));
    ASSERT_STREQ("Page2", ewk_view_title_get(webView()));
    ASSERT_TRUE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));

    // Go back to Page1
    ewk_view_back(webView());
    ASSERT_TRUE(waitUntilTitleChangedTo("Page1"));
    ASSERT_STREQ("Page1", ewk_view_title_get(webView()));
    ASSERT_FALSE(ewk_view_back_possible(webView()));
    ASSERT_TRUE(ewk_view_forward_possible(webView()));

    // Go forward to Page2
    ewk_view_forward(webView());
    ASSERT_TRUE(waitUntilTitleChangedTo("Page2"));
    ASSERT_STREQ("Page2", ewk_view_title_get(webView()));
    ASSERT_TRUE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));
}

TEST_F(EWK2UnitTestBase, DISABLED_ewk_view_setting_encoding_custom)
{
    ASSERT_FALSE(ewk_view_custom_encoding_get(webView()));
    ASSERT_TRUE(ewk_view_custom_encoding_set(webView(), "UTF-8"));
    ASSERT_STREQ("UTF-8", ewk_view_custom_encoding_get(webView()));
    // Set the default charset.
    ASSERT_TRUE(ewk_view_custom_encoding_set(webView(), 0));
    ASSERT_FALSE(ewk_view_custom_encoding_get(webView()));
}

static void onFormAboutToBeSubmitted(void* userData, Evas_Object*, void* eventInfo)
{
    Ewk_Form_Submission_Request* request = static_cast<Ewk_Form_Submission_Request*>(eventInfo);
    bool* handled = static_cast<bool*>(userData);

    ASSERT_TRUE(request);

    Eina_List* fieldNames = ewk_form_submission_request_field_names_get(request);
    ASSERT_TRUE(fieldNames);
    ASSERT_EQ(3, eina_list_count(fieldNames));
    void* data;
    EINA_LIST_FREE(fieldNames, data)
        eina_stringshare_del(static_cast<char*>(data));

    const char* value1 = ewk_form_submission_request_field_value_get(request, "text1");
    ASSERT_STREQ("value1", value1);
    eina_stringshare_del(value1);
    const char* value2 = ewk_form_submission_request_field_value_get(request, "text2");
    ASSERT_STREQ("value2", value2);
    eina_stringshare_del(value2);
    const char* password = ewk_form_submission_request_field_value_get(request, "password");
    ASSERT_STREQ("secret", password);
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

TEST_F(EWK2UnitTestBase, ewk_view_settings_get)
{
    Ewk_Settings* settings = ewk_view_settings_get(webView());
    ASSERT_TRUE(settings);
    ASSERT_EQ(settings, ewk_view_settings_get(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_view_theme_set)
{
    const char* buttonHTML = "<html><body><input type='button' id='btn'>"
        "<script>document.title=document.getElementById('btn').clientWidth;</script>"
        "</body></html>";

    ewk_view_html_string_load(webView(), buttonHTML, "file:///", 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("30")); // button of default theme has 30px as padding (15 to -16)

    ewk_view_theme_set(webView(), environment->pathForResource("it_does_not_exist.edj").data());
    ewk_view_html_string_load(webView(), buttonHTML, "file:///", 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("30")); // the result should be same as default theme

    ewk_view_theme_set(webView(), environment->pathForResource("empty_theme.edj").data());
    ewk_view_html_string_load(webView(), buttonHTML, "file:///", 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("30")); // the result should be same as default theme

    ewk_view_theme_set(webView(), environment->pathForTheme("big_button_theme.edj").data());
    ewk_view_html_string_load(webView(), buttonHTML, "file:///", 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("299")); // button of big button theme has 299px as padding (15 to -285)
}

TEST_F(EWK2UnitTestBase, ewk_view_mouse_events_enabled)
{
    ASSERT_TRUE(ewk_view_mouse_events_enabled_set(webView(), EINA_TRUE));
    ASSERT_TRUE(ewk_view_mouse_events_enabled_get(webView()));

    ASSERT_TRUE(ewk_view_mouse_events_enabled_set(webView(), 2));
    ASSERT_TRUE(ewk_view_mouse_events_enabled_get(webView()));

    ASSERT_TRUE(ewk_view_mouse_events_enabled_set(webView(), EINA_FALSE));
    ASSERT_FALSE(ewk_view_mouse_events_enabled_get(webView()));
}

static Eina_Bool fullScreenCallback(Ewk_View_Smart_Data* smartData, Ewk_Security_Origin*)
{
    fullScreenCallbackCalled = true;
    return false;
}

static Eina_Bool fullScreenExitCallback(Ewk_View_Smart_Data* smartData)
{
    fullScreenCallbackCalled = true;
    return false;
}

TEST_F(EWK2UnitTestBase, ewk_view_full_screen_enter)
{
    const char fullscreenHTML[] =
        "<!doctype html><head><script>function makeFullScreen(){"
        "var div = document.getElementById(\"fullscreen\");"
        "div.webkitRequestFullScreen();"
        "document.title = \"fullscreen entered\";"
        "}</script></head>"
        "<body><div id=\"fullscreen\" style=\"width:100px; height:100px\" onclick=\"makeFullScreen()\"></div></body>";

    ewkViewClass()->fullscreen_enter = fullScreenCallback;

    ewk_view_html_string_load(webView(), fullscreenHTML, "file:///", 0);
    ASSERT_TRUE(waitUntilLoadFinished());
    mouseClick(50, 50);
    ASSERT_TRUE(waitUntilTitleChangedTo("fullscreen entered"));
    ASSERT_TRUE(fullScreenCallbackCalled);
}

TEST_F(EWK2UnitTestBase, ewk_view_full_screen_exit)
{
    const char fullscreenHTML[] =
        "<!doctype html><head><script>function makeFullScreenAndExit(){"
        "var div = document.getElementById(\"fullscreen\");"
        "div.webkitRequestFullScreen();"
        "document.webkitCancelFullScreen();"
        "document.title = \"fullscreen exited\";"
        "}</script></head>"
        "<body><div id=\"fullscreen\" style=\"width:100px; height:100px\" onclick=\"makeFullScreenAndExit()\"></div></body>";

    ewkViewClass()->fullscreen_exit = fullScreenExitCallback;

    ewk_view_html_string_load(webView(), fullscreenHTML, "file:///", 0);
    ASSERT_TRUE(waitUntilLoadFinished());
    mouseClick(50, 50);
    ASSERT_TRUE(waitUntilTitleChangedTo("fullscreen exited"));
    ASSERT_TRUE(fullScreenCallbackCalled);
}

TEST_F(EWK2UnitTestBase, ewk_view_same_page_navigation)
{
    // Tests that same page navigation updates the page URL.
    String testUrl = environment->urlForResource("same_page_navigation.html").data();
    ASSERT_TRUE(loadUrlSync(testUrl.utf8().data()));
    ASSERT_STREQ(testUrl.utf8().data(), ewk_view_url_get(webView()));
    mouseClick(50, 50);
    testUrl = testUrl + '#';
    ASSERT_TRUE(waitUntilURLChangedTo(testUrl.utf8().data()));
}

TEST_F(EWK2UnitTestBase, ewk_view_title_changed)
{
    const char* titleChangedHTML =
        "<!doctype html><head><title>Title before changed</title></head>"
        "<body onload=\"document.title='Title after changed';\"></body>";
    ewk_view_html_string_load(webView(), titleChangedHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("Title after changed"));
    EXPECT_STREQ("Title after changed", ewk_view_title_get(webView()));

    titleChangedHTML =
        "<!doctype html><head><title>Title before changed</title></head>"
        "<body onload=\"document.title='';\"></body>";
    ewk_view_html_string_load(webView(), titleChangedHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(""));
    EXPECT_STREQ("", ewk_view_title_get(webView()));

    titleChangedHTML =
        "<!doctype html><head><title>Title before changed</title></head>"
        "<body onload=\"document.title=null;\"></body>";
    ewk_view_html_string_load(webView(), titleChangedHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(""));
    EXPECT_STREQ("", ewk_view_title_get(webView()));
}

static struct {
    const char* expectedMessage;
    bool called;
} alertCallbackData;

static struct {
    const char* expectedMessage;
    bool result;
    bool called;
} confirmCallbackData;

static struct {
    const char* expectedMessage;
    const char* expectedDefaultValue;
    const char* result;
    bool called;
} promptCallbackData;

static void checkAlert(Ewk_View_Smart_Data*, const char* message)
{
    alertCallbackData.called = true;
    EXPECT_STREQ(message, alertCallbackData.expectedMessage);
}

TEST_F(EWK2UnitTestBase, ewk_view_run_javascript_alert)
{
    ewkViewClass()->run_javascript_alert = checkAlert;

    const char* alertHTML = "<!doctype html><body onload=\"alert('Alert message');\"></body>";
    alertCallbackData.expectedMessage = "Alert message";
    alertCallbackData.called = false;
    ewk_view_html_string_load(webView(), alertHTML, 0, 0);
    EXPECT_TRUE(waitUntilLoadFinished());
    EXPECT_TRUE(alertCallbackData.called);

    alertHTML = "<!doctype html><body onload=\"alert('');\"></body>";
    alertCallbackData.expectedMessage = "";
    alertCallbackData.called = false;
    ewk_view_html_string_load(webView(), alertHTML, 0, 0);
    EXPECT_TRUE(waitUntilLoadFinished());
    EXPECT_TRUE(alertCallbackData.called);

    alertHTML = "<!doctype html><body onload=\"alert(null);\"></body>";
    alertCallbackData.expectedMessage = "null";
    alertCallbackData.called = false;
    ewk_view_html_string_load(webView(), alertHTML, 0, 0);
    EXPECT_TRUE(waitUntilLoadFinished());
    EXPECT_TRUE(alertCallbackData.called);

    alertHTML = "<!doctype html><body onload=\"alert();\"></body>";
    alertCallbackData.expectedMessage = "undefined";
    alertCallbackData.called = false;
    ewk_view_html_string_load(webView(), alertHTML, 0, 0);
    EXPECT_TRUE(waitUntilLoadFinished());
    EXPECT_TRUE(alertCallbackData.called);

    ewkViewClass()->run_javascript_alert = 0;

    alertCallbackData.called = false;
    ewk_view_html_string_load(webView(), alertHTML, 0, 0);
    EXPECT_TRUE(waitUntilLoadFinished());
    EXPECT_FALSE(alertCallbackData.called);
}

static Eina_Bool checkConfirm(Ewk_View_Smart_Data*, const char* message)
{
    confirmCallbackData.called = true;
    EXPECT_STREQ(message, confirmCallbackData.expectedMessage);
    return confirmCallbackData.result;
}

TEST_F(EWK2UnitTestBase, ewk_view_run_javascript_confirm)
{
    ewkViewClass()->run_javascript_confirm = checkConfirm;

    const char* confirmHTML = "<!doctype html><body onload=\"document.title = confirm('Confirm message');\"></body>";
    confirmCallbackData.expectedMessage = "Confirm message";
    confirmCallbackData.result = true;
    confirmCallbackData.called = false;
    ewk_view_html_string_load(webView(), confirmHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("true"));
    EXPECT_STREQ("true", ewk_view_title_get(webView()));
    EXPECT_TRUE(confirmCallbackData.called);

    confirmCallbackData.expectedMessage = "Confirm message";
    confirmCallbackData.result = false;
    confirmCallbackData.called = false;
    ewk_view_html_string_load(webView(), confirmHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("false"));
    EXPECT_STREQ("false", ewk_view_title_get(webView()));
    EXPECT_TRUE(confirmCallbackData.called);

    confirmHTML = "<!doctype html><body onload=\"document.title = confirm('');\"></body>";
    confirmCallbackData.expectedMessage = "";
    confirmCallbackData.result = true;
    confirmCallbackData.called = false;
    ewk_view_html_string_load(webView(), confirmHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("true"));
    EXPECT_STREQ("true", ewk_view_title_get(webView()));
    EXPECT_TRUE(confirmCallbackData.called);

    confirmHTML = "<!doctype html><body onload=\"document.title = confirm(null);\"></body>";
    confirmCallbackData.expectedMessage = "null";
    confirmCallbackData.result = true;
    confirmCallbackData.called = false;
    ewk_view_html_string_load(webView(), confirmHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("true"));
    EXPECT_STREQ("true", ewk_view_title_get(webView()));
    EXPECT_TRUE(confirmCallbackData.called);

    confirmHTML = "<!doctype html><body onload=\"document.title = confirm();\"></body>";
    confirmCallbackData.expectedMessage = "undefined";
    confirmCallbackData.result = true;
    confirmCallbackData.called = false;
    ewk_view_html_string_load(webView(), confirmHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("true"));
    EXPECT_STREQ("true", ewk_view_title_get(webView()));
    EXPECT_TRUE(confirmCallbackData.called);

    ewkViewClass()->run_javascript_confirm = 0;

    confirmCallbackData.called = false;
    ewk_view_html_string_load(webView(), confirmHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("false"));
    EXPECT_STREQ("false", ewk_view_title_get(webView()));
    EXPECT_FALSE(confirmCallbackData.called);
}

static const char* checkPrompt(Ewk_View_Smart_Data*, const char* message, const char* defaultValue)
{
    promptCallbackData.called = true;
    EXPECT_STREQ(message, promptCallbackData.expectedMessage);
    EXPECT_STREQ(defaultValue, promptCallbackData.expectedDefaultValue);

    if (!promptCallbackData.result)
        return 0;

    return eina_stringshare_add(promptCallbackData.result);
}

TEST_F(EWK2UnitTestBase, ewk_view_run_javascript_prompt)
{
    static const char promptMessage[] = "Prompt message";
    static const char promptResult[] = "Prompt result";

    ewkViewClass()->run_javascript_prompt = checkPrompt;

    const char* promptHTML = "<!doctype html><body onload=\"document.title = prompt('Prompt message', 'Prompt default value');\"></body>";
    promptCallbackData.expectedMessage = promptMessage;
    promptCallbackData.expectedDefaultValue = "Prompt default value";
    promptCallbackData.result = promptResult;
    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(promptResult));
    EXPECT_STREQ(promptResult, ewk_view_title_get(webView()));
    EXPECT_TRUE(promptCallbackData.called);

    promptHTML = "<!doctype html><body onload=\"document.title = prompt('Prompt message', '');\"></body>";
    promptCallbackData.expectedMessage = promptMessage;
    promptCallbackData.expectedDefaultValue = "";
    promptCallbackData.result = promptResult;
    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(promptResult));
    EXPECT_STREQ(promptResult, ewk_view_title_get(webView()));
    EXPECT_TRUE(promptCallbackData.called);

    promptHTML = "<!doctype html><body onload=\"document.title = prompt('Prompt message');\"></body>";
    promptCallbackData.expectedMessage = promptMessage;
    promptCallbackData.expectedDefaultValue = "";
    promptCallbackData.result = promptResult;
    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(promptResult));
    EXPECT_STREQ(promptResult, ewk_view_title_get(webView()));
    EXPECT_TRUE(promptCallbackData.called);

    promptHTML = "<!doctype html><body onload=\"document.title = prompt('');\"></body>";
    promptCallbackData.expectedMessage = "";
    promptCallbackData.expectedDefaultValue = "";
    promptCallbackData.result = promptResult;
    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(promptResult));
    EXPECT_STREQ(promptResult, ewk_view_title_get(webView()));
    EXPECT_TRUE(promptCallbackData.called);

    promptHTML = "<!doctype html><body onload=\"document.title = prompt();\"></body>";
    promptCallbackData.expectedMessage = "undefined";
    promptCallbackData.expectedDefaultValue = "";
    promptCallbackData.result = promptResult;
    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(promptResult));
    EXPECT_STREQ(promptResult, ewk_view_title_get(webView()));
    EXPECT_TRUE(promptCallbackData.called);

    promptHTML = "<html><head><title>Default title</title></head>"
                 "<body onload=\"var promptResult = prompt('Prompt message');"
                 "if (promptResult == null) document.title='null';"
                 "else document.title = promptResult;\"></body></html>";
    promptCallbackData.expectedMessage = promptMessage;
    promptCallbackData.expectedDefaultValue = "";
    promptCallbackData.result = "";
    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo(""));
    EXPECT_STREQ("", ewk_view_title_get(webView()));
    EXPECT_TRUE(promptCallbackData.called);

    promptCallbackData.expectedMessage = promptMessage;
    promptCallbackData.expectedDefaultValue = "";
    promptCallbackData.result = 0;
    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("null"));
    EXPECT_STREQ("null", ewk_view_title_get(webView()));
    EXPECT_TRUE(promptCallbackData.called);

    ewkViewClass()->run_javascript_prompt = 0;

    promptCallbackData.called = false;
    ewk_view_html_string_load(webView(), promptHTML, 0, 0);
    EXPECT_TRUE(waitUntilTitleChangedTo("null"));
    EXPECT_STREQ("null", ewk_view_title_get(webView()));
    EXPECT_FALSE(promptCallbackData.called);
}

TEST_F(EWK2UnitTestBase, ewk_view_context_get)
{
    Ewk_Context* context = ewk_view_context_get(webView());
    ASSERT_TRUE(context);
    ASSERT_EQ(context, ewk_view_context_get(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_view_feed_touch_event)
{
    Eina_List* points = 0;
    Ewk_Touch_Point point1 = { 0, 0, 0, EVAS_TOUCH_POINT_DOWN };
    Ewk_Touch_Point point2 = { 1, 0, 0, EVAS_TOUCH_POINT_DOWN };
    points = eina_list_append(points, &point1);
    points = eina_list_append(points, &point2);
    ASSERT_TRUE(ewk_view_feed_touch_event(webView(), EWK_TOUCH_START, points, evas_key_modifier_get(evas_object_evas_get(webView()))));

    point1.state = EVAS_TOUCH_POINT_STILL;
    point2.x = 100;
    point2.y = 100;
    point2.state = EVAS_TOUCH_POINT_MOVE;
    ASSERT_TRUE(ewk_view_feed_touch_event(webView(), EWK_TOUCH_MOVE, points, evas_key_modifier_get(evas_object_evas_get(webView()))));

    point2.state = EVAS_TOUCH_POINT_UP;
    ASSERT_TRUE(ewk_view_feed_touch_event(webView(), EWK_TOUCH_END, points, evas_key_modifier_get(evas_object_evas_get(webView()))));
    points = eina_list_remove(points, &point2);

    point1.state = EVAS_TOUCH_POINT_CANCEL;
    ASSERT_TRUE(ewk_view_feed_touch_event(webView(), EWK_TOUCH_CANCEL, points, evas_key_modifier_get(evas_object_evas_get(webView()))));
    points = eina_list_remove(points, &point1);

    eina_list_free(points);
}

static void onTextFound(void* userData, Evas_Object*, void* eventInfo)
{
    int* result = static_cast<int*>(userData);
    unsigned* matchCount = static_cast<unsigned*>(eventInfo);

    *result = *matchCount;
}

TEST_F(EWK2UnitTestBase, ewk_view_text_find)
{
    const char textFindHTML[] =
        "<!DOCTYPE html>"
        "<body>"
        "apple apple apple banana banana coconut"
        "</body>";
    ewk_view_html_string_load(webView(), textFindHTML, 0, 0);
    waitUntilLoadFinished();

    int matchCount = -1;
    evas_object_smart_callback_add(webView(), "text,found", onTextFound, &matchCount);

    ewk_view_text_find(webView(), "apple", EWK_FIND_OPTIONS_SHOW_OVERLAY, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(3, matchCount);

    matchCount = -1;
    ewk_view_text_find(webView(), "mango", EWK_FIND_OPTIONS_SHOW_OVERLAY, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(0, matchCount);

    evas_object_smart_callback_del(webView(), "text,found", onTextFound);
}

TEST_F(EWK2UnitTestBase, ewk_view_text_matches_count)
{
    const char textFindHTML[] =
        "<!DOCTYPE html>"
        "<body>"
        "apple Apple apple apple banana bananaApple banana coconut"
        "</body>";
    ewk_view_html_string_load(webView(), textFindHTML, 0, 0);
    waitUntilLoadFinished();

    int matchCount = -1;
    evas_object_smart_callback_add(webView(), "text,found", onTextFound, &matchCount);

    ewk_view_text_matches_count(webView(), "apple", EWK_FIND_OPTIONS_NONE, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(3, matchCount);

    matchCount = -1;
    ewk_view_text_matches_count(webView(), "apple", EWK_FIND_OPTIONS_CASE_INSENSITIVE, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(5, matchCount);

    matchCount = -1;
    ewk_view_text_matches_count(webView(), "Apple", EWK_FIND_OPTIONS_AT_WORD_STARTS, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(1, matchCount);

    matchCount = -1;
    ewk_view_text_matches_count(webView(), "Apple", EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(2, matchCount);

    matchCount = -1;
    ewk_view_text_matches_count(webView(), "mango", EWK_FIND_OPTIONS_NONE, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(0, matchCount);

    // If we have more matches than allowed, -1 is returned as a matched count.
    matchCount = -2;
    ewk_view_text_matches_count(webView(), "apple", EWK_FIND_OPTIONS_NONE, 2);
    while (matchCount < -1)
        ecore_main_loop_iterate();
    EXPECT_EQ(-1, matchCount);

    evas_object_smart_callback_del(webView(), "text,found", onTextFound);
}

TEST_F(EWK2UnitTestBase, ewk_view_touch_events_enabled)
{
    ASSERT_FALSE(ewk_view_touch_events_enabled_get(webView()));

    ASSERT_TRUE(ewk_view_touch_events_enabled_set(webView(), true));
    ASSERT_TRUE(ewk_view_touch_events_enabled_get(webView()));

    ASSERT_TRUE(ewk_view_touch_events_enabled_set(webView(), 2));
    ASSERT_TRUE(ewk_view_touch_events_enabled_get(webView()));

    const char* touchHTML =
        "<!doctype html><body><div style=\"width:100px; height:100px;\" "
        "ontouchstart=\"document.title='touchstarted' + event.touches.length;\" "
        "ontouchmove=\"document.title='touchmoved' + event.touches.length;\" "
        "ontouchend=\"document.title='touchended' + event.touches.length;\">"
        "</div></body>";

    ewk_view_html_string_load(webView(), touchHTML, "file:///", 0);
    ASSERT_TRUE(waitUntilLoadFinished());

    mouseDown(10, 10);
    ASSERT_TRUE(waitUntilTitleChangedTo("touchstarted1"));

    multiDown(1, 30, 30);
    ASSERT_TRUE(waitUntilTitleChangedTo("touchstarted2"));

    multiMove(1, 40, 40);
    ASSERT_TRUE(waitUntilTitleChangedTo("touchmoved2"));

    multiUp(1, 40, 40);
    ASSERT_TRUE(waitUntilTitleChangedTo("touchended1"));

    mouseMove(20, 20);
    ASSERT_TRUE(waitUntilTitleChangedTo("touchmoved1"));

    mouseUp(20, 20);
    ASSERT_TRUE(waitUntilTitleChangedTo("touchended0"));

    ASSERT_TRUE(ewk_view_touch_events_enabled_set(webView(), false));
    ASSERT_FALSE(ewk_view_touch_events_enabled_get(webView()));
}

Eina_Bool windowMoveResizeTimedOut(void* data)
{
    *static_cast<bool*>(data) = true;
}

TEST_F(EWK2UnitTestBase, window_move_resize)
{
    int x, y, width, height;
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_object_evas_get(webView()));
    ecore_evas_geometry_get(ee, 0, 0, &width, &height);

    EXPECT_EQ(800, width);
    EXPECT_EQ(600, height);

    ewk_view_url_set(webView(), environment->urlForResource("window_move_resize.html").data());
    ASSERT_TRUE(waitUntilTitleChangedTo("Moved and resized"));

    // Check that the window has been moved and resized.
    ecore_evas_request_geometry_get(ee, &x, &y, &width, &height);
    EXPECT_EQ(150, x);
    EXPECT_EQ(200, y);
    EXPECT_EQ(200, width);
    EXPECT_EQ(100, height);
}

TEST_F(EWK2UnitTestBase, ewk_view_inspector)
{
#if ENABLE(INSPECTOR)
    ASSERT_TRUE(ewk_view_inspector_show(webView()));
    ASSERT_TRUE(ewk_view_inspector_close(webView()));
#else
    ASSERT_FALSE(ewk_view_inspector_show(webView()));
    ASSERT_FALSE(ewk_view_inspector_close(webView()));
#endif
}

TEST_F(EWK2UnitTestBase, ewk_view_scale)
{
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));

    // Default scale value is 1.0.
    ASSERT_FLOAT_EQ(1, ewk_view_scale_get(webView()));

    ASSERT_TRUE(ewk_view_scale_set(webView(), 1.2, 0, 0));
    // Reload page to check the page scale value.
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_FLOAT_EQ(1.2, ewk_view_scale_get(webView()));

    ASSERT_TRUE(ewk_view_scale_set(webView(), 1.5, 0, 0));
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_FLOAT_EQ(1.5, ewk_view_scale_get(webView()));

    ASSERT_TRUE(ewk_view_scale_set(webView(), 1, 0, 0));
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_FLOAT_EQ(1, ewk_view_scale_get(webView()));
}

TEST_F(EWK2UnitTestBase, ewk_view_pagination)
{
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));

    // Default pagination value is EWK_PAGINATION_MODE_UNPAGINATED
    ASSERT_EQ(EWK_PAGINATION_MODE_UNPAGINATED, ewk_view_pagination_mode_get(webView()));

    ASSERT_TRUE(ewk_view_pagination_mode_set(webView(), EWK_PAGINATION_MODE_LEFT_TO_RIGHT));
    // Reload page to check the pagination mode.
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_EQ(EWK_PAGINATION_MODE_LEFT_TO_RIGHT, ewk_view_pagination_mode_get(webView()));

    ASSERT_TRUE(ewk_view_pagination_mode_set(webView(), EWK_PAGINATION_MODE_RIGHT_TO_LEFT));
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_EQ(EWK_PAGINATION_MODE_RIGHT_TO_LEFT, ewk_view_pagination_mode_get(webView()));

    ASSERT_TRUE(ewk_view_pagination_mode_set(webView(), EWK_PAGINATION_MODE_TOP_TO_BOTTOM));
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_EQ(EWK_PAGINATION_MODE_TOP_TO_BOTTOM, ewk_view_pagination_mode_get(webView()));

    ASSERT_TRUE(ewk_view_pagination_mode_set(webView(),  EWK_PAGINATION_MODE_BOTTOM_TO_TOP));
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_EQ(EWK_PAGINATION_MODE_BOTTOM_TO_TOP, ewk_view_pagination_mode_get(webView()));

    ASSERT_TRUE(ewk_view_pagination_mode_set(webView(), EWK_PAGINATION_MODE_UNPAGINATED));
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ASSERT_EQ(EWK_PAGINATION_MODE_UNPAGINATED, ewk_view_pagination_mode_get(webView()));
}

struct VibrationCbData {
    bool didReceiveVibrate; // Whether the vibration event received.
    bool didReceiveCancelVibration; // Whether the cancel vibration event received.
    unsigned vibrateCalledCount; // Vibrate callbacks count.
    uint64_t expectedVibrationTime; // Expected vibration time.
};

static void onVibrate(void* userData, Evas_Object*, void* eventInfo)
{
    VibrationCbData* data = static_cast<VibrationCbData*>(userData);
    uint64_t* vibrationTime = static_cast<uint64_t*>(eventInfo);
    if (*vibrationTime == data->expectedVibrationTime)
        data->didReceiveVibrate = true;
    data->vibrateCalledCount++;
}

static void onCancelVibration(void* userData, Evas_Object*, void*)
{
    VibrationCbData* data = static_cast<VibrationCbData*>(userData);
    data->didReceiveCancelVibration = true;
}

static void loadVibrationHTMLString(Evas_Object* webView, const char* vibrationPattern, bool waitForVibrationEvent, VibrationCbData* data)
{
    const char* content =
        "<html><head><script type='text/javascript'>function vibrate() { navigator.vibrate(%s);"
        " document.title = \"Loaded\"; }</script></head><body onload='vibrate()'></body></html>";

    data->didReceiveVibrate = false;
    data->didReceiveCancelVibration = false;
    data->vibrateCalledCount = 0;
    Eina_Strbuf* buffer = eina_strbuf_new();
    eina_strbuf_append_printf(buffer, content, vibrationPattern);
    ewk_view_html_string_load(webView, eina_strbuf_string_get(buffer), 0, 0);
    eina_strbuf_free(buffer);

    if (!waitForVibrationEvent)
        return;

    while (!data->didReceiveVibrate && !data->didReceiveCancelVibration)
        ecore_main_loop_iterate();
}

TEST_F(EWK2UnitTestBase, ewk_context_vibration_client_callbacks_set)
{
    VibrationCbData data = { false, false, 0, 5000 };
    evas_object_smart_callback_add(webView(), "vibrate", onVibrate, &data);
    evas_object_smart_callback_add(webView(), "cancel,vibration", onCancelVibration, &data);

    // Vibrate for 5 seconds.
    loadVibrationHTMLString(webView(), "5000", true, &data);
    ASSERT_TRUE(data.didReceiveVibrate);

    // Cancel any existing vibrations.
    loadVibrationHTMLString(webView(), "0", true, &data);
    ASSERT_TRUE(data.didReceiveCancelVibration);

    // This case the pattern will cause the device to vibrate for 200 ms, be still for 100 ms, and then vibrate for 5000 ms.
    loadVibrationHTMLString(webView(), "[200, 100, 5000]", true, &data);
    ASSERT_EQ(2, data.vibrateCalledCount);
    ASSERT_TRUE(data.didReceiveVibrate);

    // Cancel outstanding vibration pattern.
    loadVibrationHTMLString(webView(), "[0]", true, &data);
    ASSERT_TRUE(data.didReceiveCancelVibration);

    // Stop listening for vibration events, by calling the function with null for the callbacks.
    evas_object_smart_callback_del(webView(), "vibrate", onVibrate);
    evas_object_smart_callback_del(webView(), "cancel,vibration", onCancelVibration);

    // Make sure we don't receive vibration event.
    loadVibrationHTMLString(webView(), "[5000]", false, &data);
    ASSERT_TRUE(waitUntilTitleChangedTo("Loaded"));
    ASSERT_STREQ("Loaded", ewk_view_title_get(webView()));
    ASSERT_FALSE(data.didReceiveVibrate);

    // Make sure we don't receive cancel vibration event.
    loadVibrationHTMLString(webView(), "0", false, &data);
    ASSERT_TRUE(waitUntilTitleChangedTo("Loaded"));
    ASSERT_STREQ("Loaded", ewk_view_title_get(webView()));
    ASSERT_FALSE(data.didReceiveCancelVibration);
}

