/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_view.h"
#include "ewk_view_private.h"

#include "EwkView.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_context_private.h"
#include "ewk_main_private.h"
#include "ewk_page_group_private.h"
#include <JavaScriptCore/JSRetainPtr.h>
#include <WebKit/WKAPICast.h>
#include <WebKit/WKData.h>
#include <WebKit/WKEinaSharedString.h>
#include <WebKit/WKFindOptions.h>
#include <WebKit/WKInspector.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKSerializedScriptValue.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>
#include <WebKit/WKView.h>
#include <WebKit/WKViewEfl.h>
#include <wtf/text/CString.h>

#if ENABLE(INSPECTOR)
#include "WebInspectorProxy.h"
#endif

using namespace WebKit;

static inline EwkView* toEwkViewChecked(const Evas_Object* evasObject)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(evasObject, nullptr);
    if (EINA_UNLIKELY(!isEwkViewEvasObject(evasObject)))
        return 0;

    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evasObject));
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData, nullptr);

    return smartData->priv;
}

#define EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, ...)                        \
    EwkView* impl = toEwkViewChecked(ewkView);                                 \
    do {                                                                       \
        if (EINA_UNLIKELY(!impl)) {                                            \
            EINA_LOG_CRIT("no private data for object %p", ewkView);           \
            return __VA_ARGS__;                                                \
        }                                                                      \
    } while (0)


Eina_Bool ewk_view_smart_class_set(Ewk_View_Smart_Class* api)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(api, false);

    return EwkView::initSmartClassInterface(*api);
}

Evas_Object* EWKViewCreate(WKContextRef context, WKPageGroupRef pageGroup, Evas* canvas, Evas_Smart* smart)
{
    if (!EwkMain::shared().isInitialized()) {
        EINA_LOG_CRIT("EWebKit has not been initialized. You must call ewk_init() before creating view.");
        return nullptr;
    }

    WKRetainPtr<WKViewRef> wkView = adoptWK(WKViewCreate(context, pageGroup));
    if (EwkView* ewkView = EwkView::create(wkView.get(), canvas, smart))
        return ewkView->evasObject();

    return nullptr;
}

WKViewRef EWKViewGetWKView(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->wkView();
}

Evas_Object* ewk_view_smart_add(Evas* canvas, Evas_Smart* smart, Ewk_Context* context, Ewk_Page_Group* pageGroup)
{
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);
    EwkPageGroup* ewkPageGroup = ewk_object_cast<EwkPageGroup*>(pageGroup);

    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, nullptr);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext->wkContext(), nullptr);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkPageGroup, nullptr);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkPageGroup->wkPageGroup(), nullptr);

    return EWKViewCreate(ewkContext->wkContext(), ewkPageGroup->wkPageGroup(), canvas, smart);
}

Evas_Object* ewk_view_add(Evas* canvas)
{
    return ewk_view_add_with_context(canvas, ewk_context_default_get());
}

Evas_Object* ewk_view_add_with_context(Evas* canvas, Ewk_Context* context)
{
    EwkContext* ewkContext = ewk_object_cast<EwkContext*>(context);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, nullptr);
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext->wkContext(), nullptr);

    return EWKViewCreate(ewkContext->wkContext(), 0, canvas, 0);
}

Ewk_Context* ewk_view_context_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->ewkContext();
}

Ewk_Page_Group* ewk_view_page_group_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->ewkPageGroup();
}

Eina_Bool ewk_view_url_set(Evas_Object* ewkView, const char* url)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);

    WKRetainPtr<WKURLRef> wkUrl = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(impl->wkPage(), wkUrl.get());
    impl->informURLChange();

    return true;
}

const char* ewk_view_url_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->url();
}

Eina_Bool ewk_view_reload(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageReload(impl->wkPage());
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_reload_bypass_cache(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageReloadFromOrigin(impl->wkPage());
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_stop(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageStopLoading(impl->wkPage());

    return true;
}

const char* ewk_view_title_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->title();
}

double ewk_view_load_progress_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1.0);

    return WKPageGetEstimatedProgress(impl->wkPage());
}

Eina_Bool ewk_view_scale_set(Evas_Object* ewkView, double scaleFactor, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetScaleFactor(impl->wkPage(), scaleFactor, WKPointMake(x, y));
    return true;
}

double ewk_view_scale_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1);

    return WKPageGetScaleFactor(impl->wkPage());
}

Eina_Bool ewk_view_page_zoom_set(Evas_Object* ewkView, double zoomFactor)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetPageZoomFactor(impl->wkPage(), zoomFactor);

    return true;
}

double ewk_view_page_zoom_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1);

    return WKPageGetPageZoomFactor(impl->wkPage());
}

Eina_Bool ewk_view_device_pixel_ratio_set(Evas_Object* ewkView, float ratio)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setDeviceScaleFactor(ratio);

    return true;
}

float ewk_view_device_pixel_ratio_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, -1.0);

    return WKPageGetBackingScaleFactor(impl->wkPage());
}

void ewk_view_theme_set(Evas_Object* ewkView, const char* path)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->setThemePath(path);
}

const char* ewk_view_theme_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->themePath();
}

Eina_Bool ewk_view_back(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageRef page = impl->wkPage();
    if (WKPageCanGoBack(page)) {
        WKPageGoBack(page);
        return true;
    }

    return false;
}

Eina_Bool ewk_view_forward(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageRef page = impl->wkPage();
    if (WKPageCanGoForward(page)) {
        WKPageGoForward(page);
        return true;
    }

    return false;
}

Eina_Bool ewk_view_back_possible(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    return WKPageCanGoBack(impl->wkPage());
}

Eina_Bool ewk_view_forward_possible(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    return WKPageCanGoForward(impl->wkPage());
}

Ewk_Back_Forward_List* ewk_view_back_forward_list_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->backForwardList();
}

Eina_Bool ewk_view_navigate_to(Evas_Object* ewkView, const Ewk_Back_Forward_List_Item* item)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkBackForwardListItem, item, itemImpl, false);

    WKPageGoToBackForwardListItem(impl->wkPage(), itemImpl->wkItem());

    return true;
}

Eina_Bool ewk_view_html_string_load(Evas_Object* ewkView, const char* html, const char* baseUrl, const char* unreachableUrl)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(html, false);

    WKRetainPtr<WKStringRef> wkHTMLString = adoptWK(WKStringCreateWithUTF8CString(html));
    WKRetainPtr<WKURLRef> wkBaseURL = adoptWK(WKURLCreateWithUTF8CString(baseUrl));

    if (unreachableUrl && *unreachableUrl) {
        WKRetainPtr<WKURLRef> wkUnreachableURL = adoptWK(WKURLCreateWithUTF8CString(unreachableUrl));
        WKPageLoadAlternateHTMLString(impl->wkPage(), wkHTMLString.get(), wkBaseURL.get(), wkUnreachableURL.get());
    } else
        WKPageLoadHTMLString(impl->wkPage(), wkHTMLString.get(), wkBaseURL.get());

    impl->informURLChange();

    return true;
}

const char* ewk_view_custom_encoding_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->customTextEncodingName();
}

Eina_Bool ewk_view_custom_encoding_set(Evas_Object* ewkView, const char* encoding)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setCustomTextEncodingName(encoding);

    return true;
}

const char* ewk_view_user_agent_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->userAgent();
}

Eina_Bool ewk_view_user_agent_set(Evas_Object* ewkView, const char* userAgent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setUserAgent(userAgent);

    return true;
}

Eina_Bool ewk_view_application_name_for_user_agent_set(Evas_Object* ewkView, const char* applicationName)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setApplicationNameForUserAgent(applicationName);
    return true;
}

const char* ewk_view_application_name_for_user_agent_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, nullptr);

    return impl->applicationNameForUserAgent();
}

inline WKFindOptions toWKFindOptions(Ewk_Find_Options options)
{
    unsigned wkFindOptions = 0;

    if (options & EWK_FIND_OPTIONS_CASE_INSENSITIVE)
        wkFindOptions |= kWKFindOptionsCaseInsensitive;
    if (options & EWK_FIND_OPTIONS_AT_WORD_STARTS)
        wkFindOptions |= kWKFindOptionsAtWordStarts;
    if (options & EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START)
        wkFindOptions |= kWKFindOptionsTreatMedialCapitalAsWordStart;
    if (options & EWK_FIND_OPTIONS_BACKWARDS)
        wkFindOptions |= kWKFindOptionsBackwards;
    if (options & EWK_FIND_OPTIONS_WRAP_AROUND)
        wkFindOptions |= kWKFindOptionsWrapAround;
    if (options & EWK_FIND_OPTIONS_SHOW_OVERLAY)
        wkFindOptions |= kWKFindOptionsShowOverlay;
    if (options & EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR)
        wkFindOptions |= kWKFindOptionsShowFindIndicator;
    if (options & EWK_FIND_OPTIONS_SHOW_HIGHLIGHT)
        wkFindOptions |= kWKFindOptionsShowHighlight;

    return static_cast<WKFindOptions>(wkFindOptions);
}

Eina_Bool ewk_view_text_find(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    WKRetainPtr<WKStringRef> wkText = adoptWK(WKStringCreateWithUTF8CString(text));
    WKPageFindString(impl->wkPage(), wkText.get(), toWKFindOptions(options), maxMatchCount);

    return true;
}

Eina_Bool ewk_view_text_find_highlight_clear(Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageHideFindUI(impl->wkPage());

    return true;
}

Eina_Bool ewk_view_text_matches_count(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    WKRetainPtr<WKStringRef> wkText = adoptWK(WKStringCreateWithUTF8CString(text));
    WKPageCountStringMatches(impl->wkPage(), wkText.get(), static_cast<WebKit::FindOptions>(options), maxMatchCount);

    return true;
}

Eina_Bool ewk_view_mouse_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setMouseEventsEnabled(!!enabled);

    return true;
}

Eina_Bool ewk_view_mouse_events_enabled_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->mouseEventsEnabled();
}

Eina_Bool ewk_view_feed_touch_event(Evas_Object* ewkView, Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers)
{
#if ENABLE(TOUCH_EVENTS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(points, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->feedTouchEvent(type, points, modifiers);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(type);
    UNUSED_PARAM(points);
    UNUSED_PARAM(modifiers);
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    impl->setTouchEventsEnabled(!!enabled);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(enabled);
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_get(const Evas_Object* ewkView)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return impl->touchEventsEnabled();
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_inspector_show(Evas_Object* ewkView)
{
#if ENABLE(INSPECTOR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKInspectorRef wkInspector = WKPageGetInspector(impl->wkPage());
    if (wkInspector)
        WKInspectorShow(wkInspector);

    return true;
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_inspector_close(Evas_Object* ewkView)
{
#if ENABLE(INSPECTOR)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKInspectorRef wkInspector = WKPageGetInspector(impl->wkPage());
    if (wkInspector)
        WKInspectorClose(wkInspector);

    return true;
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

inline WKPaginationMode toWKPaginationMode(Ewk_Pagination_Mode mode)
{
    switch (mode) {
    case EWK_PAGINATION_MODE_INVALID:
        break;
    case EWK_PAGINATION_MODE_UNPAGINATED:
        return kWKPaginationModeUnpaginated;
    case EWK_PAGINATION_MODE_LEFT_TO_RIGHT:
        return kWKPaginationModeLeftToRight;
    case EWK_PAGINATION_MODE_RIGHT_TO_LEFT:
        return kWKPaginationModeRightToLeft;
    case EWK_PAGINATION_MODE_TOP_TO_BOTTOM:
        return kWKPaginationModeTopToBottom;
    case EWK_PAGINATION_MODE_BOTTOM_TO_TOP:
        return kWKPaginationModeBottomToTop;
    }
    ASSERT_NOT_REACHED();

    return kWKPaginationModeUnpaginated;
}

inline Ewk_Pagination_Mode toEwkPaginationMode(WKPaginationMode mode)
{
    switch (mode) {
    case kWKPaginationModeUnpaginated:
        return EWK_PAGINATION_MODE_UNPAGINATED;
    case kWKPaginationModeLeftToRight:
        return EWK_PAGINATION_MODE_LEFT_TO_RIGHT;
    case kWKPaginationModeRightToLeft:
        return EWK_PAGINATION_MODE_RIGHT_TO_LEFT;
    case kWKPaginationModeTopToBottom:
        return EWK_PAGINATION_MODE_TOP_TO_BOTTOM;
    case kWKPaginationModeBottomToTop:
        return EWK_PAGINATION_MODE_BOTTOM_TO_TOP;
    }
    ASSERT_NOT_REACHED();

    return EWK_PAGINATION_MODE_UNPAGINATED;
}

Eina_Bool ewk_view_pagination_mode_set(Evas_Object* ewkView, Ewk_Pagination_Mode mode)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetPaginationMode(impl->wkPage(), toWKPaginationMode(mode));

    return true;
}

Ewk_Pagination_Mode ewk_view_pagination_mode_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, EWK_PAGINATION_MODE_INVALID);
    
    return toEwkPaginationMode(WKPageGetPaginationMode(impl->wkPage()));
}

Eina_Bool ewk_view_fullscreen_exit(Evas_Object* ewkView)
{
#if ENABLE(FULLSCREEN_API)
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return WKViewExitFullScreen(impl->wkView());
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

/// Creates a type name for Ewk_Page_Contents_Context.
typedef struct Ewk_Page_Contents_Context Ewk_Page_Contents_Context;

/*
 * @brief Structure containing page contents context used for ewk_view_page_contents_get() API.
 */
struct Ewk_Page_Contents_Context {
    Ewk_Page_Contents_Type type;
    Ewk_Page_Contents_Cb callback;
    void* userData;
};

/**
 * @internal
 * Callback function used for ewk_view_page_contents_get().
 */
static void ewkViewPageContentsAsMHTMLCallback(WKDataRef wkData, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    auto contentsContext = std::unique_ptr<Ewk_Page_Contents_Context>(static_cast<Ewk_Page_Contents_Context*>(context));
    contentsContext->callback(contentsContext->type, reinterpret_cast<const char*>(WKDataGetBytes(wkData)), contentsContext->userData);
}

/**
 * @internal
 * Callback function used for ewk_view_page_contents_get().
 */
static void ewkViewPageContentsAsStringCallback(WKStringRef wkString, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);

    auto contentsContext = std::unique_ptr<Ewk_Page_Contents_Context>(static_cast<Ewk_Page_Contents_Context*>(context));
    contentsContext->callback(contentsContext->type, WKEinaSharedString(wkString), contentsContext->userData);
}

Eina_Bool ewk_view_page_contents_get(const Evas_Object* ewkView, Ewk_Page_Contents_Type type, Ewk_Page_Contents_Cb callback, void* user_data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    Ewk_Page_Contents_Context* context = new Ewk_Page_Contents_Context;
    context->type = type;
    context->callback = callback;
    context->userData = user_data;

    switch (context->type) {
    case EWK_PAGE_CONTENTS_TYPE_MHTML:
        WKPageGetContentsAsMHTMLData(impl->wkPage(), false, context, ewkViewPageContentsAsMHTMLCallback);
        break;
    case EWK_PAGE_CONTENTS_TYPE_STRING:
        WKPageGetContentsAsString(impl->wkPage(), context, ewkViewPageContentsAsStringCallback);
        break;
    default:
        delete context;
        ASSERT_NOT_REACHED();
        return false;
    }

    return true;
}

struct Ewk_View_Script_Execute_Callback_Context {
    Ewk_View_Script_Execute_Callback_Context(Ewk_View_Script_Execute_Cb callback, Evas_Object* ewkView, void* userData)
        : m_callback(callback)
        , m_view(ewkView)
        , m_userData(userData)
    {
    }

    Ewk_View_Script_Execute_Cb m_callback;
    Evas_Object* m_view;
    void* m_userData;
};

static void runJavaScriptCallback(WKSerializedScriptValueRef scriptValue, WKErrorRef, void* context)
{
    ASSERT(context);

    auto callbackContext = std::unique_ptr<Ewk_View_Script_Execute_Callback_Context>(static_cast<Ewk_View_Script_Execute_Callback_Context*>(context));
    ASSERT(callbackContext->m_view);

    if (!callbackContext->m_callback)
        return;

    if (scriptValue) {
        EWK_VIEW_IMPL_GET_OR_RETURN(callbackContext->m_view, impl);
        JSGlobalContextRef jsGlobalContext = impl->ewkContext()->jsGlobalContext();

        JSValueRef value = WKSerializedScriptValueDeserialize(scriptValue, jsGlobalContext, 0);
        JSRetainPtr<JSStringRef> jsStringValue(Adopt, JSValueToStringCopy(jsGlobalContext, value, 0));
        size_t length = JSStringGetMaximumUTF8CStringSize(jsStringValue.get());
        auto buffer = std::make_unique<char[]>(length);
        JSStringGetUTF8CString(jsStringValue.get(), buffer.get(), length);
        callbackContext->m_callback(callbackContext->m_view, buffer.get(), callbackContext->m_userData);
    } else
        callbackContext->m_callback(callbackContext->m_view, 0, callbackContext->m_userData);
}

Eina_Bool ewk_view_script_execute(Evas_Object* ewkView, const char* script, Ewk_View_Script_Execute_Cb callback, void* userData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(script, false);

    Ewk_View_Script_Execute_Callback_Context* context = new Ewk_View_Script_Execute_Callback_Context(callback, ewkView, userData);
    WKRetainPtr<WKStringRef> scriptString(AdoptWK, WKStringCreateWithUTF8CString(script));
    WKPageRunJavaScriptInMainFrame(impl->wkPage(), scriptString.get(), context, runJavaScriptCallback);
    return true;
}

Eina_Bool ewk_view_layout_fixed_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    WKPageSetUseFixedLayout(WKViewGetPage(impl->wkView()), enabled);

    return true;
}

Eina_Bool ewk_view_layout_fixed_get(const Evas_Object* ewkView)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl, false);

    return WKPageUseFixedLayout(WKViewGetPage(impl->wkView()));
}

void ewk_view_layout_fixed_size_set(const Evas_Object* ewkView, Evas_Coord width, Evas_Coord height)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKPageSetFixedLayoutSize(WKViewGetPage(impl->wkView()), WKSizeMake(width, height));
}

void ewk_view_layout_fixed_size_get(const Evas_Object* ewkView, Evas_Coord* width, Evas_Coord* height)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKSize size = WKPageFixedLayoutSize(WKViewGetPage(impl->wkView()));

    if (width)
        *width = size.width;
    if (height)
        *height = size.height;
}

void ewk_view_bg_color_set(Evas_Object* ewkView, int red, int green, int blue, int alpha)
{
    if (EINA_UNLIKELY(alpha < 0 || alpha > 255)) {
        EINA_LOG_CRIT("Alpha should be between 0 and 255");
        return;
    }

    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    impl->setBackgroundColor(red, green, blue, alpha);
}

void ewk_view_bg_color_get(const Evas_Object* ewkView, int* red, int* green, int* blue, int* alpha)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(ewkView, impl);

    WKViewGetBackgroundColor(impl->wkView(), red, green, blue, alpha);
}

Eina_Bool ewk_view_contents_size_get(const Evas_Object* ewkView, Evas_Coord* width, Evas_Coord* height)
{
    EwkView* impl = toEwkViewChecked(ewkView);
    if (EINA_UNLIKELY(!impl)) {
        EINA_LOG_CRIT("no private data for object %p", ewkView);
        if (width)
            *width = 0;
        if (height)
            *height = 0;

        return false;
    }

    WKSize contentsSize = WKViewGetContentsSize(impl->wkView());
    if (width)
        *width = contentsSize.width;
    if (height)
        *height = contentsSize.height;

    return true;
}
