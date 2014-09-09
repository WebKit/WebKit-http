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
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

static bool fullScreenCallbackCalled;
static bool obtainedPageContents = false;
static bool scriptExecuteCallbackCalled;

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

class EWK2ViewTest : public EWK2UnitTestBase {
public:
    struct VibrationCbData {
        bool didReceiveVibrate; // Whether the vibration event received.
        bool didReceiveCancelVibration; // Whether the cancel vibration event received.
        bool testFinished;
        unsigned vibrateCalledCount; // Vibrate callbacks count.
        unsigned expectedVibrationTime; // Expected vibration time.
    };

    static void onLoadFinishedForRedirection(void* userData, Evas_Object*, void*)
    {
        int* countLoadFinished = static_cast<int*>(userData);
        (*countLoadFinished)--;
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

    static void checkAlert(Ewk_View_Smart_Data*, const char* message)
    {
        alertCallbackData.called = true;
        EXPECT_STREQ(message, alertCallbackData.expectedMessage);
    }

    static Eina_Bool checkConfirm(Ewk_View_Smart_Data*, const char* message)
    {
        confirmCallbackData.called = true;
        EXPECT_STREQ(message, confirmCallbackData.expectedMessage);
        return confirmCallbackData.result;
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

    static void onTextFound(void* userData, Evas_Object*, void* eventInfo)
    {
        int* result = static_cast<int*>(userData);
        unsigned* matchCount = static_cast<unsigned*>(eventInfo);

        *result = *matchCount;
    }

    static void onVibrate(void* userData, Evas_Object*, void* eventInfo)
    {
        VibrationCbData* data = static_cast<VibrationCbData*>(userData);
        unsigned* vibrationTime = static_cast<unsigned*>(eventInfo);
        if (*vibrationTime == data->expectedVibrationTime) {
            data->didReceiveVibrate = true;
            data->testFinished = true;
        }
        data->vibrateCalledCount++;
    }

    static void onCancelVibration(void* userData, Evas_Object*, void*)
    {
        VibrationCbData* data = static_cast<VibrationCbData*>(userData);
        data->didReceiveCancelVibration = true;
        data->testFinished = true;
    }

    static void loadVibrationHTMLString(Evas_Object* webView, const char* vibrationPattern, VibrationCbData* data)
    {
        const char* content =
            "<html><head><script type='text/javascript'>function vibrate() { navigator.vibrate(%s);"
            " document.title = \"Loaded\"; }</script></head><body onload='vibrate()'></body></html>";

        data->didReceiveVibrate = false;
        data->didReceiveCancelVibration = false;
        data->vibrateCalledCount = 0;
        data->testFinished = false;
        Eina_Strbuf* buffer = eina_strbuf_new();
        eina_strbuf_append_printf(buffer, content, vibrationPattern);
        ewk_view_html_string_load(webView, eina_strbuf_string_get(buffer), 0, 0);
        eina_strbuf_free(buffer);
    }

    static void onContentsSizeChangedPortrait(void* userData, Evas_Object*, void* eventInfo)
    {
        bool* result = static_cast<bool*>(userData);
        Ewk_CSS_Size* size = static_cast<Ewk_CSS_Size*>(eventInfo);
        if (size->w == 2000 && size->h == 3000)
            *result = true;
    }

    static void onContentsSizeChangedLandscape(void* userData, Evas_Object*, void* eventInfo)
    {
        bool* result = static_cast<bool*>(userData);
        Ewk_CSS_Size* size = static_cast<Ewk_CSS_Size*>(eventInfo);
        if (size->w == 3000 && size->h == 2000)
            *result = true;
    }

    static void PageContentsAsMHTMLCallback(Ewk_Page_Contents_Type type, const char* data, void*)
    {
        // Check the type
        ASSERT_EQ(EWK_PAGE_CONTENTS_TYPE_MHTML, type);

        // We should have exactly the same amount of bytes in the file
        // than those coming from the callback data. We don't compare the
        // strings read since the 'Date' field and the boundaries will be
        // different on each case. MHTML functionality will be tested by
        // layout tests, so checking the amount of bytes is enough.
        Eina_File* f = eina_file_open(TEST_RESOURCES_DIR "/resultMHTML.mht", false);
        if (!f)
            return;

        size_t fileSize = eina_file_size_get(f);
        // As the day in 'Date' field may be one digit or two digit, the data length may also be varied by one byte.
        EXPECT_TRUE(String(data).length() == fileSize || String(data).length() == fileSize + 1);

        obtainedPageContents = true;
        eina_file_close(f);
    }

    static void PageContentsAsStringCallback(Ewk_Page_Contents_Type type, const char* data, void*)
    {
        // Check the type.
        ASSERT_EQ(EWK_PAGE_CONTENTS_TYPE_STRING, type);

        // The variable data should be "Simple HTML".
        ASSERT_STREQ("Simple HTML", data);

        obtainedPageContents = true;
    }

    static void FocusNotFoundCallback(void* userData, Evas_Object*, void* eventInfo)
    {
        Ewk_Focus_Direction* direction = static_cast<Ewk_Focus_Direction*>(eventInfo);
        Ewk_Focus_Direction* result = static_cast<Ewk_Focus_Direction*>(userData);
        *result = *direction;
    }
};

TEST_F(EWK2ViewTest, ewk_view_type_check)
{
    ASSERT_FALSE(ewk_view_context_get(0));

    Evas_Object* rectangle = evas_object_rectangle_add(canvas());
    ASSERT_FALSE(ewk_view_url_set(rectangle, 0));
}

TEST_F(EWK2ViewTest, ewk_view_add)
{
    Evas_Object* view = ewk_view_add(canvas());
    ASSERT_EQ(ewk_context_default_get(), ewk_view_context_get(view));

    EXPECT_TRUE(ewk_view_stop(view));
    evas_object_del(view);
}

TEST_F(EWK2ViewTest, ewk_view_url_get)
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

TEST_F(EWK2ViewTest, ewk_view_device_pixel_ratio)
{
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));

    // Default pixel ratio is 1.0
    ASSERT_FLOAT_EQ(1, ewk_view_device_pixel_ratio_get(webView()));

    ASSERT_TRUE(ewk_view_device_pixel_ratio_set(webView(), 1.2));
    ASSERT_FLOAT_EQ(1.2, ewk_view_device_pixel_ratio_get(webView()));

    ASSERT_TRUE(ewk_view_device_pixel_ratio_set(webView(), 1));
    ASSERT_FLOAT_EQ(1, ewk_view_device_pixel_ratio_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_html_string_load)
{
    ewk_view_html_string_load(webView(), "<html><head><title>Foo</title></head><body>Bar</body></html>", 0, 0);
    ASSERT_TRUE(waitUntilTitleChangedTo("Foo"));
    ASSERT_STREQ("Foo", ewk_view_title_get(webView()));
    ewk_view_html_string_load(webView(), "<html><head><title>Bar</title></head><body>Foo</body></html>", 0, 0);
    ASSERT_TRUE(waitUntilTitleChangedTo("Bar"));
    ASSERT_STREQ("Bar", ewk_view_title_get(webView()));

    // Checking in case of null for html and webview
    ASSERT_FALSE(ewk_view_html_string_load(0, "<html><head><title>Foo</title></head><body>Bar</body></html>", 0, 0));
    ASSERT_FALSE(ewk_view_html_string_load(webView(), 0, 0, 0));
}

TEST_F(EWK2ViewTest, ewk_view_navigation)
{
    // Visit Page1
    ewk_view_url_set(webView(), environment->urlForResource("/Page1.html").data());
    ASSERT_TRUE(waitUntilTitleChangedTo("Page1"));
    ASSERT_STREQ("Page1", ewk_view_title_get(webView()));
    ASSERT_FALSE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));

    // Visit Page2
    ewk_view_url_set(webView(), environment->urlForResource("/Page2.html").data());
    ASSERT_TRUE(waitUntilTitleChangedTo("Page2"));
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

    // Visit Page3
    ewk_view_url_set(webView(), environment->urlForResource("/Page3.html").data());
    ASSERT_TRUE(waitUntilTitleChangedTo("Page3"));
    ASSERT_STREQ("Page3", ewk_view_title_get(webView()));
    ASSERT_TRUE(ewk_view_back_possible(webView()));
    ASSERT_FALSE(ewk_view_forward_possible(webView()));

    Ewk_Back_Forward_List* list = ewk_view_back_forward_list_get(webView());
    ASSERT_EQ(3, ewk_back_forward_list_count(list));

    // Navigate to Page1
    ewk_view_navigate_to(webView(), ewk_back_forward_list_item_at_index_get(list, -2));
    ASSERT_TRUE(waitUntilTitleChangedTo("Page1"));
    ASSERT_STREQ("Page1", ewk_view_title_get(webView()));

    // Navigate to Page3
    ewk_view_navigate_to(webView(), ewk_back_forward_list_item_at_index_get(list, 2));
    ASSERT_TRUE(waitUntilTitleChangedTo("Page3"));
    ASSERT_STREQ("Page3", ewk_view_title_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_custom_encoding)
{
    ASSERT_FALSE(ewk_view_custom_encoding_get(webView()));
    ASSERT_TRUE(ewk_view_custom_encoding_set(webView(), "UTF-8"));
    ASSERT_STREQ("UTF-8", ewk_view_custom_encoding_get(webView()));
    // Set the default charset.
    ASSERT_TRUE(ewk_view_custom_encoding_set(webView(), 0));
    ASSERT_FALSE(ewk_view_custom_encoding_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_form_submission_request)
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
    ASSERT_TRUE(waitUntilTrue(handled));
    evas_object_smart_callback_del(webView(), "form,submission,request", onFormAboutToBeSubmitted);
}

TEST_F(EWK2ViewTest, ewk_view_theme_set)
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

    EXPECT_STREQ(environment->pathForTheme("big_button_theme.edj").data(), ewk_view_theme_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_mouse_events_enabled)
{
    ASSERT_TRUE(ewk_view_mouse_events_enabled_set(webView(), EINA_TRUE));
    ASSERT_TRUE(ewk_view_mouse_events_enabled_get(webView()));

    ASSERT_TRUE(ewk_view_mouse_events_enabled_set(webView(), 2));
    ASSERT_TRUE(ewk_view_mouse_events_enabled_get(webView()));

    ASSERT_TRUE(ewk_view_mouse_events_enabled_set(webView(), EINA_FALSE));
    ASSERT_FALSE(ewk_view_mouse_events_enabled_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_full_screen_enter)
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

TEST_F(EWK2ViewTest, ewk_view_full_screen_exit)
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

TEST_F(EWK2ViewTest, ewk_view_cancel_full_screen_request)
{
    // FullScreenmanager should skip cancel fullscreen request if fullscreen
    // mode was not set using FullScreen API.
    ASSERT_FALSE(ewk_view_fullscreen_exit(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_same_page_navigation)
{
    // Tests that same page navigation updates the page URL.
    String testUrl = environment->urlForResource("same_page_navigation.html").data();
    ASSERT_TRUE(loadUrlSync(testUrl.utf8().data()));
    ASSERT_STREQ(testUrl.utf8().data(), ewk_view_url_get(webView()));
    mouseClick(50, 50);
    testUrl = testUrl + '#';
    ASSERT_TRUE(waitUntilURLChangedTo(testUrl.utf8().data()));
}

TEST_F(EWK2ViewTest, ewk_view_title_changed)
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

TEST_F(EWK2ViewTest, ewk_view_run_javascript_alert)
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

TEST_F(EWK2ViewTest, ewk_view_run_javascript_confirm)
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

TEST_F(EWK2ViewTest, ewk_view_run_javascript_prompt)
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

TEST_F(EWK2ViewTest, ewk_view_context_get)
{
    Ewk_Context* context = ewk_view_context_get(webView());
    ASSERT_TRUE(context);
    ASSERT_EQ(context, ewk_view_context_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_page_group_get)
{
    Ewk_Page_Group* pageGroup = ewk_view_page_group_get(webView());
    ASSERT_TRUE(pageGroup);
    ASSERT_EQ(pageGroup, ewk_view_page_group_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_feed_touch_event)
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

    // Checking in case of null for points and webview
    EXPECT_FALSE(ewk_view_feed_touch_event(0, EWK_TOUCH_CANCEL, points, evas_key_modifier_get(evas_object_evas_get(webView()))));
    EXPECT_FALSE(ewk_view_feed_touch_event(webView(), EWK_TOUCH_CANCEL, 0, evas_key_modifier_get(evas_object_evas_get(webView()))));

    eina_list_free(points);
}

TEST_F(EWK2ViewTest, ewk_view_text_find)
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

    EXPECT_TRUE(ewk_view_text_find_highlight_clear(webView()));

    matchCount = -1;
    ewk_view_text_find(webView(), "mango", EWK_FIND_OPTIONS_SHOW_OVERLAY, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(0, matchCount);

    matchCount = -1;
    ewk_view_text_find(webView(), "banana", EWK_FIND_OPTIONS_SHOW_HIGHLIGHT, 100);
    while (matchCount < 0)
        ecore_main_loop_iterate();
    EXPECT_EQ(2, matchCount);

    // Checking in case of null for text and webview
    ASSERT_FALSE(ewk_view_text_find(0, "apple", EWK_FIND_OPTIONS_SHOW_OVERLAY, 100));
    ASSERT_FALSE(ewk_view_text_find(webView(), 0, EWK_FIND_OPTIONS_SHOW_OVERLAY, 100));

    evas_object_smart_callback_del(webView(), "text,found", onTextFound);
}

TEST_F(EWK2ViewTest, ewk_view_text_matches_count)
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

    // Checking in case of null for text and webview
    EXPECT_FALSE(ewk_view_text_matches_count(0, "apple", EWK_FIND_OPTIONS_NONE, 100));
    EXPECT_FALSE(ewk_view_text_matches_count(webView(), 0, EWK_FIND_OPTIONS_NONE, 100));

    evas_object_smart_callback_del(webView(), "text,found", onTextFound);
}

TEST_F(EWK2ViewTest, ewk_view_touch_events_enabled)
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

TEST_F(EWK2ViewTest, window_move_resize)
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

TEST_F(EWK2ViewTest, ewk_view_inspector)
{
#if ENABLE(INSPECTOR)
    ASSERT_TRUE(ewk_view_inspector_show(webView()));
    ASSERT_TRUE(ewk_view_inspector_close(webView()));
#else
    ASSERT_FALSE(ewk_view_inspector_show(webView()));
    ASSERT_FALSE(ewk_view_inspector_close(webView()));
#endif
}

TEST_F(EWK2ViewTest, DISABLED_ewk_view_scale)
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

TEST_F(EWK2ViewTest, ewk_view_pagination)
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

TEST_F(EWK2ViewTest, ewk_context_vibration_client_callbacks_set)
{
    VibrationCbData data = { false, false, false, 0, 5000 };
    evas_object_smart_callback_add(webView(), "vibrate", onVibrate, &data);
    evas_object_smart_callback_add(webView(), "cancel,vibration", onCancelVibration, &data);

    // Vibrate for 5 seconds.
    loadVibrationHTMLString(webView(), "5000", &data);
    waitUntilTrue(data.testFinished);
    ASSERT_TRUE(data.didReceiveVibrate && !data.didReceiveCancelVibration);

    // Cancel any existing vibrations.
    loadVibrationHTMLString(webView(), "0", &data);
    waitUntilTrue(data.testFinished);
    ASSERT_TRUE(!data.didReceiveVibrate && data.didReceiveCancelVibration);

    // This case the pattern will cause the device to vibrate for 200 ms, be still for 100 ms, and then vibrate for 5000 ms.
    loadVibrationHTMLString(webView(), "[200, 100, 5000]", &data);
    waitUntilTrue(data.testFinished);
    ASSERT_EQ(2, data.vibrateCalledCount);
    ASSERT_TRUE(data.didReceiveVibrate && !data.didReceiveCancelVibration);

    // Cancel outstanding vibration pattern.
    loadVibrationHTMLString(webView(), "[0]", &data);
    waitUntilTrue(data.testFinished);
    ASSERT_TRUE(!data.didReceiveVibrate && data.didReceiveCancelVibration);

    // Check that vibration works properly by continuous request.
    data.expectedVibrationTime = 5;
    loadVibrationHTMLString(webView(), "5", &data);
    waitUntilTrue(data.testFinished);
    ASSERT_TRUE(data.didReceiveVibrate && !data.didReceiveCancelVibration);

    loadVibrationHTMLString(webView(), "5", &data);
    waitUntilTrue(data.testFinished);
    ASSERT_EQ(1, data.vibrateCalledCount);
    ASSERT_TRUE(data.didReceiveVibrate && !data.didReceiveCancelVibration);

    // Stop listening for vibration events, by calling the function with null for the callbacks.
    evas_object_smart_callback_del(webView(), "vibrate", onVibrate);
    evas_object_smart_callback_del(webView(), "cancel,vibration", onCancelVibration);
}

TEST_F(EWK2ViewTest, ewk_view_contents_size_changed)
{
    const char contentsSizeHTMLPortrait[] =
        "<!DOCTYPE html>"
        "<body style=\"margin:0px;width:2000px;height:3000px\"></body>";
    const char contentsSizeHTMLLandscape[] =
        "<!DOCTYPE html>"
        "<body style=\"margin:0px;width:3000px;height:2000px\"></body>";

    bool sizeChanged = false;
    evas_object_smart_callback_add(webView(), "contents,size,changed", onContentsSizeChangedPortrait, &sizeChanged);
    ewk_view_html_string_load(webView(), contentsSizeHTMLPortrait, 0, 0);
    waitUntilTrue(sizeChanged);
    evas_object_smart_callback_del(webView(), "contents,size,changed", onContentsSizeChangedPortrait);

    evas_object_smart_callback_add(webView(), "contents,size,changed", onContentsSizeChangedLandscape, &sizeChanged);
    ewk_view_device_pixel_ratio_set(webView(), 2);
    ewk_view_html_string_load(webView(), contentsSizeHTMLLandscape, 0, 0);
    sizeChanged = false;
    waitUntilTrue(sizeChanged);
    evas_object_smart_callback_del(webView(), "contents,size,changed", onContentsSizeChangedLandscape);

    evas_object_smart_callback_add(webView(), "contents,size,changed", onContentsSizeChangedPortrait, &sizeChanged);
    ewk_view_scale_set(webView(), 3, 0, 0);
    ewk_view_html_string_load(webView(), contentsSizeHTMLPortrait, 0, 0);
    sizeChanged = false;
    waitUntilTrue(sizeChanged);

    // Make sure we get signal after loaded the contents having same size with previous one.
    sizeChanged = false;
    ewk_view_html_string_load(webView(), contentsSizeHTMLPortrait, 0, 0);
    waitUntilTrue(sizeChanged);

    evas_object_smart_callback_del(webView(), "contents,size,changed", onContentsSizeChangedPortrait);
}

TEST_F(EWK2ViewTest, ewk_view_page_contents_get)
{
    const char content[] = "<p>Simple HTML</p>";
    ewk_view_html_string_load(webView(), content, 0, 0);
    waitUntilLoadFinished();

    ASSERT_TRUE(ewk_view_page_contents_get(webView(), EWK_PAGE_CONTENTS_TYPE_MHTML, PageContentsAsMHTMLCallback, 0));
    waitUntilTrue(obtainedPageContents);

    obtainedPageContents = false;
    ASSERT_TRUE(ewk_view_page_contents_get(webView(), EWK_PAGE_CONTENTS_TYPE_STRING, PageContentsAsStringCallback, 0));
    waitUntilTrue(obtainedPageContents);

    // Checking in case of null for callback and webview
    EXPECT_FALSE(ewk_view_page_contents_get(webView(), EWK_PAGE_CONTENTS_TYPE_STRING, 0, 0));
    EXPECT_FALSE(ewk_view_page_contents_get(0, EWK_PAGE_CONTENTS_TYPE_STRING, PageContentsAsStringCallback, 0));
    EXPECT_FALSE(ewk_view_page_contents_get(webView(), static_cast<Ewk_Page_Contents_Type>(2), PageContentsAsStringCallback, 0));
}

TEST_F(EWK2ViewTest, ewk_view_user_agent)
{
    const char* defaultUserAgent = eina_stringshare_add(ewk_view_user_agent_get(webView()));
    const char customUserAgent[] = "Foo";

    ASSERT_TRUE(ewk_view_user_agent_set(webView(), customUserAgent));
    ASSERT_STREQ(customUserAgent, ewk_view_user_agent_get(webView()));

    // Set the default user agent string.
    ASSERT_TRUE(ewk_view_user_agent_set(webView(), 0));
    ASSERT_STREQ(defaultUserAgent, ewk_view_user_agent_get(webView()));
    eina_stringshare_del(defaultUserAgent);
}

TEST_F(EWK2ViewTest, ewk_view_application_name_for_user_agent)
{
    const char customUserAgent[] = "Foo";
    const char applicationNameForUserAgent[] = "Bar";

    ASSERT_TRUE(ewk_view_user_agent_set(webView(), customUserAgent));

    ASSERT_TRUE(ewk_view_application_name_for_user_agent_set(webView(), applicationNameForUserAgent));
    ASSERT_STREQ(applicationNameForUserAgent, ewk_view_application_name_for_user_agent_get(webView()));
    ASSERT_STREQ(customUserAgent, ewk_view_user_agent_get(webView()));
    ASSERT_FALSE(strstr(ewk_view_user_agent_get(webView()), applicationNameForUserAgent));

    ASSERT_TRUE(ewk_view_user_agent_set(webView(), nullptr));
    ASSERT_TRUE(strstr(ewk_view_user_agent_get(webView()), applicationNameForUserAgent));
}

static void scriptExecuteCallback(Evas_Object*, const char* returnValue, void* userData)
{
    Eina_Strbuf* buffer = static_cast<Eina_Strbuf*>(userData);
    eina_strbuf_reset(buffer);

    if (returnValue)
        eina_strbuf_append(buffer, returnValue);

    scriptExecuteCallbackCalled = true;
}

TEST_F(EWK2UnitTestBase, ewk_view_script_execute)
{
    const char scriptExecuteHTML[] =
        "<!DOCTYPE html>"
        "<body>"
        "<p id=\"TestContent\">test content</p>"
        "</body>";

    ewk_view_html_string_load(webView(), scriptExecuteHTML, 0, 0);
    ASSERT_TRUE(waitUntilLoadFinished());

    Eina_Strbuf* result = eina_strbuf_new();

    // 1. Get the innerHTML for "TestContent"
    const char getDataScript[] = "document.getElementById('TestContent').innerHTML";

    scriptExecuteCallbackCalled = false;
    ASSERT_TRUE(ewk_view_script_execute(webView(), getDataScript, scriptExecuteCallback, static_cast<void*>(result)));
    waitUntilTrue(scriptExecuteCallbackCalled);
    ASSERT_STREQ("test content", eina_strbuf_string_get(result));

    // 2. Change the innerHTML for "TestContent"
    const char changeDataScript[] =
    "document.getElementById('TestContent').innerHTML = \"test\";";
    ASSERT_TRUE(ewk_view_script_execute(webView(), changeDataScript, 0, 0));

    // 3. Check the change of the innerHTML.
    eina_strbuf_reset(result);
    scriptExecuteCallbackCalled = false;
    ASSERT_TRUE(ewk_view_script_execute(webView(), getDataScript, scriptExecuteCallback, static_cast<void*>(result)));
    waitUntilTrue(scriptExecuteCallbackCalled);
    ASSERT_STREQ("test", eina_strbuf_string_get(result));

    // Checking in case of null for script and webview
    EXPECT_FALSE(ewk_view_script_execute(0, getDataScript, scriptExecuteCallback, static_cast<void*>(result)));
    EXPECT_FALSE(ewk_view_script_execute(webView(), 0, scriptExecuteCallback, static_cast<void*>(result)));

    eina_strbuf_free(result);

    ASSERT_FALSE(ewk_view_script_execute(webView(), 0, 0, 0));
}

TEST_F(EWK2ViewTest, ewk_view_layout_fixed)
{
    // Fixed layout is not used in webview as default.
    EXPECT_FALSE(ewk_view_layout_fixed_get(webView()));

    EXPECT_TRUE(ewk_view_layout_fixed_set(webView(), true));
    EXPECT_TRUE(ewk_view_layout_fixed_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_layout_fixed_size)
{    
    // Fixed layout is not enabled in webview as default.
    EXPECT_FALSE(ewk_view_layout_fixed_get(webView()));
    EXPECT_TRUE(ewk_view_layout_fixed_set(webView(), true));

    Evas_Coord width = 0;
    Evas_Coord height = 0;

    ewk_view_layout_fixed_size_set(webView(), 480, 800);
    ewk_view_layout_fixed_size_get(webView(), &width, &height);
    EXPECT_EQ(480, width);
    EXPECT_EQ(800, height);

    ewk_view_layout_fixed_size_set(webView(), 980, 1020);
    ewk_view_layout_fixed_size_get(webView(), &width, &height);
    EXPECT_EQ(980, width);
    EXPECT_EQ(1020, height);
}

TEST_F(EWK2ViewTest, ewk_view_bg_color)
{
    const char noBackgroundHTML[] = "<!doctype html><body></body>";

    ewk_view_bg_color_set(webView(), 255, 0, 0, 255);
    ewk_view_html_string_load(webView(), noBackgroundHTML, 0, 0);
    ASSERT_TRUE(waitUntilLoadFinished());

    int red, green, blue, alpha;
    ewk_view_bg_color_get(webView(), &red, &green, &blue, &alpha);
    ASSERT_EQ(255, red);
    ASSERT_EQ(0, green);
    ASSERT_EQ(0, blue);
    ASSERT_EQ(255, red);
}

TEST_F(EWK2ViewTest, ewk_view_page_zoom_set)
{
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));

    // Default zoom factor is 1.0
    ASSERT_FLOAT_EQ(1, ewk_view_page_zoom_get(webView()));

    ASSERT_TRUE(ewk_view_page_zoom_set(webView(), 0.67));
    ASSERT_FLOAT_EQ(0.67, ewk_view_page_zoom_get(webView()));

    ASSERT_TRUE(ewk_view_page_zoom_set(webView(), 1));
    ASSERT_FLOAT_EQ(1, ewk_view_page_zoom_get(webView()));
}

TEST_F(EWK2ViewTest, ewk_view_contents_size_get)
{
    int contentsWidth, contentsHeight;
    ewk_view_contents_size_get(0, &contentsWidth, &contentsHeight);

    EXPECT_EQ(0, contentsWidth);
    EXPECT_EQ(0, contentsHeight);

    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ewk_view_contents_size_get(webView(), &contentsWidth, &contentsHeight);

    EXPECT_EQ(environment->defaultWidth(), contentsWidth);
    EXPECT_EQ(environment->defaultHeight(), contentsHeight);

    ewk_view_device_pixel_ratio_set(webView(), 2);
    ASSERT_TRUE(loadUrlSync(environment->defaultTestPageUrl()));
    ewk_view_contents_size_get(webView(), &contentsWidth, &contentsHeight);

    EXPECT_EQ(environment->defaultWidth() / 2, contentsWidth);
    EXPECT_EQ(environment->defaultHeight() / 2, contentsHeight);

    const char fixedContentsSize[] =
        "<!DOCTYPE html>"
        "<body style=\"margin:0px;width:2000px;height:3000px\"></body>";
    ewk_view_html_string_load(webView(), fixedContentsSize, 0, 0);
    ASSERT_TRUE(waitUntilLoadFinished());
    ewk_view_contents_size_get(webView(), &contentsWidth, &contentsHeight);

    EXPECT_EQ(2000, contentsWidth);
    EXPECT_EQ(3000, contentsHeight);
}

TEST_F(EWK2ViewTest, ewk_focus_notfound)
{
    const char contents[] =
        "<!DOCTYPE html>"
        "<body><input type='text' autofocus></body>";
    ewk_view_html_string_load(webView(), contents, 0, 0);
    ASSERT_TRUE(waitUntilLoadFinished());

    Ewk_Settings* settings = ewk_page_group_settings_get(ewk_view_page_group_get(webView()));
    ewk_settings_spatial_navigation_enabled_set(settings, EINA_TRUE);

    Ewk_Focus_Direction direction = EWK_FOCUS_DIRECTION_FORWARD;
    evas_object_smart_callback_add(webView(), "focus,notfound", FocusNotFoundCallback, &direction);

    keyDown("Tab", "Tab", 0, "Shift");
    keyUp("Tab", "Tab", 0);

    ASSERT_TRUE(waitUntilDirectionChanged(direction));
    EXPECT_EQ(EWK_FOCUS_DIRECTION_BACKWARD, direction);

    // Set focus to the input element again.
    keyDown("Tab", "Tab", 0, 0);
    keyUp("Tab", "Tab", 0);

    keyDown("Tab", "Tab", 0, 0);
    keyUp("Tab", "Tab", 0);

    ASSERT_TRUE(waitUntilDirectionChanged(direction));
    EXPECT_EQ(EWK_FOCUS_DIRECTION_FORWARD, direction);

    evas_object_smart_callback_del(webView(), "focus,notfound", FocusNotFoundCallback);
}
