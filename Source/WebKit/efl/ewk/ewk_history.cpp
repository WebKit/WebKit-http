/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

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
#include "ewk_history.h"

#include "BackForwardListImpl.h"
#include "EWebKit.h"
#include "HistoryItem.h"
#include "IconDatabaseBase.h"
#include "Image.h"
#include "IntSize.h"
#include "ewk_private.h"

#include <Eina.h>
#include <eina_safety_checks.h>
#include <wtf/text/CString.h>

struct _Ewk_History {
    WebCore::BackForwardListImpl *core;
};

#define EWK_HISTORY_CORE_GET_OR_RETURN(history, core_, ...)      \
    if (!(history)) {                                            \
        CRITICAL("history is NULL.");                            \
        return __VA_ARGS__;                                      \
    }                                                            \
    if (!(history)->core) {                                      \
        CRITICAL("history->core is NULL.");                      \
        return __VA_ARGS__;                                      \
    }                                                            \
    if (!(history)->core->enabled()) {                           \
        ERR("history->core is disabled!.");                      \
        return __VA_ARGS__;                                      \
    }                                                            \
    WebCore::BackForwardListImpl *core_ = (history)->core


struct _Ewk_History_Item {
    WebCore::HistoryItem *core;

    const char *title;
    const char *alternate_title;
    const char *uri;
    const char *original_uri;
};

#define EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core_, ...) \
    if (!(item)) {                                            \
        CRITICAL("item is NULL.");                            \
        return __VA_ARGS__;                                   \
    }                                                         \
    if (!(item)->core) {                                      \
        CRITICAL("item->core is NULL.");                      \
        return __VA_ARGS__;                                   \
    }                                                         \
    WebCore::HistoryItem *core_ = (item)->core


static inline Ewk_History_Item *_ewk_history_item_new(WebCore::HistoryItem *core)
{
    Ewk_History_Item* item;

    if (!core) {
        ERR("WebCore::HistoryItem is NULL.");
        return 0;
    }

    item = (Ewk_History_Item *)calloc(1, sizeof(Ewk_History_Item));
    if (!item) {
        CRITICAL("Could not allocate item memory.");
        return 0;
    }

    core->ref();
    item->core = core;

    return item;
}

static inline Eina_List *_ewk_history_item_list_get(const WebCore::HistoryItemVector &core_items)
{
    Eina_List* ret = 0;
    unsigned int i, size;

    size = core_items.size();
    for (i = 0; i < size; i++) {
        Ewk_History_Item* item = _ewk_history_item_new(core_items[i].get());
        if (item)
            ret = eina_list_append(ret, item);
    }

    return ret;
}

Eina_Bool ewk_history_forward(Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, EINA_FALSE);
    if (core->forwardListCount() < 1)
        return EINA_FALSE;
    core->goForward();
    return EINA_TRUE;
}

Eina_Bool ewk_history_back(Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, EINA_FALSE);
    if (core->backListCount() < 1)
        return EINA_FALSE;
    core->goBack();
    return EINA_TRUE;
}

Eina_Bool ewk_history_history_item_add(Ewk_History* history, const Ewk_History_Item* item)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, history_core, EINA_FALSE);
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, item_core, EINA_FALSE);
    history_core->addItem(item_core);
    return EINA_TRUE;
}

Eina_Bool ewk_history_history_item_set(Ewk_History* history, const Ewk_History_Item* item)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, history_core, EINA_FALSE);
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, item_core, EINA_FALSE);
    history_core->goToItem(item_core);
    return EINA_TRUE;
}

Ewk_History_Item* ewk_history_history_item_back_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return _ewk_history_item_new(core->backItem());
}

Ewk_History_Item* ewk_history_history_item_current_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return _ewk_history_item_new(core->currentItem());
}

Ewk_History_Item* ewk_history_history_item_forward_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return _ewk_history_item_new(core->forwardItem());
}

Ewk_History_Item* ewk_history_history_item_nth_get(const Ewk_History* history, int index)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return _ewk_history_item_new(core->itemAtIndex(index));
}

Eina_Bool ewk_history_history_item_contains(const Ewk_History* history, const Ewk_History_Item* item)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, history_core, EINA_FALSE);
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, item_core, EINA_FALSE);
    return history_core->containsItem(item_core);
}

Eina_List* ewk_history_forward_list_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    int limit = core->forwardListCount();
    core->forwardListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

Eina_List* ewk_history_forward_list_get_with_limit(const Ewk_History* history, int limit)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    core->forwardListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

int ewk_history_forward_list_length(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return core->forwardListCount();
}

Eina_List* ewk_history_back_list_get(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    int limit = core->backListCount();
    core->backListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

Eina_List* ewk_history_back_list_get_with_limit(const Ewk_History* history, int limit)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    WebCore::HistoryItemVector items;
    core->backListWithLimit(limit, items);
    return _ewk_history_item_list_get(items);
}

int ewk_history_back_list_length(const Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return core->backListCount();
}

int ewk_history_limit_get(Ewk_History* history)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, 0);
    return core->capacity();
}

Eina_Bool ewk_history_limit_set(const Ewk_History* history, int limit)
{
    EWK_HISTORY_CORE_GET_OR_RETURN(history, core, EINA_FALSE);
    core->setCapacity(limit);
    return EINA_TRUE;
}

Ewk_History_Item* ewk_history_item_new(const char* uri, const char* title)
{
    WTF::String u = WTF::String::fromUTF8(uri);
    WTF::String t = WTF::String::fromUTF8(title);
    WTF::RefPtr<WebCore::HistoryItem> core = WebCore::HistoryItem::create(u, t, 0);
    Ewk_History_Item* item = _ewk_history_item_new(core.release().releaseRef());
    return item;
}

static inline void _ewk_history_item_free(Ewk_History_Item* item, WebCore::HistoryItem* core)
{
    core->deref();
    free(item);
}

void ewk_history_item_free(Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core);
    _ewk_history_item_free(item, core);
}

void ewk_history_item_list_free(Eina_List* history_items)
{
    void* d;
    EINA_LIST_FREE(history_items, d) {
        Ewk_History_Item* item = (Ewk_History_Item*)d;
        _ewk_history_item_free(item, item->core);
    }
}

const char* ewk_history_item_title_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* i = (Ewk_History_Item*)item;
    eina_stringshare_replace(&i->title, core->title().utf8().data());
    return i->title;
}

const char* ewk_history_item_title_alternate_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* i = (Ewk_History_Item*)item;
    eina_stringshare_replace(&i->alternate_title,
                             core->alternateTitle().utf8().data());
    return i->alternate_title;
}

void ewk_history_item_title_alternate_set(Ewk_History_Item* item, const char* title)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core);
    if (!eina_stringshare_replace(&item->alternate_title, title))
        return;
    core->setAlternateTitle(WTF::String::fromUTF8(title));
}

const char* ewk_history_item_uri_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* i = (Ewk_History_Item*)item;
    eina_stringshare_replace(&i->uri, core->urlString().utf8().data());
    return i->uri;
}

const char* ewk_history_item_uri_original_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    // hide the following optimzation from outside
    Ewk_History_Item* i = (Ewk_History_Item*)item;
    eina_stringshare_replace(&i->original_uri,
                             core->originalURLString().utf8().data());
    return i->original_uri;
}

double ewk_history_item_time_last_visited_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0.0);
    return core->lastVisitedTime();
}

cairo_surface_t* ewk_history_item_icon_surface_get(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    
    WebCore::Image* icon = WebCore::iconDatabase().synchronousIconForPageURL(core->url(), WebCore::IntSize(16, 16));
    if (!icon) {
        ERR("icon is NULL.");
        return 0;
    }
    return icon->nativeImageForCurrentFrame();
}

Evas_Object* ewk_history_item_icon_object_add(const Ewk_History_Item* item, Evas* canvas)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);
    WebCore::Image* icon = WebCore::iconDatabase().synchronousIconForPageURL(core->url(), WebCore::IntSize(16, 16));
    cairo_surface_t* surface;

    if (!icon) {
        ERR("icon is NULL.");
        return 0;
    }

    surface = icon->nativeImageForCurrentFrame();
    return ewk_util_image_from_cairo_surface_add(canvas, surface);
}

Eina_Bool ewk_history_item_page_cache_exists(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, EINA_FALSE);
    return core->isInPageCache();
}

int ewk_history_item_visit_count(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, 0);
    return core->visitCount();
}

Eina_Bool ewk_history_item_visit_last_failed(const Ewk_History_Item* item)
{
    EWK_HISTORY_ITEM_CORE_GET_OR_RETURN(item, core, EINA_TRUE);
    return core->lastVisitWasFailure();
}


/* internal methods ****************************************************/
/**
 * @internal
 *
 * Creates history for given view. Called internally by ewk_view and
 * should never be called from outside.
 *
 * @param core WebCore::BackForwardListImpl instance to use internally.
 *
 * @return newly allocated history instance or @c NULL on errors.
 */
Ewk_History* ewk_history_new(WebCore::BackForwardListImpl* core)
{
    Ewk_History* history;
    EINA_SAFETY_ON_NULL_RETURN_VAL(core, 0);
    DBG("core=%p", core);

    history = (Ewk_History*)malloc(sizeof(Ewk_History));
    if (!history) {
        CRITICAL("Could not allocate history memory.");
        return 0;
    }

    core->ref();
    history->core = core;

    return history;
}

/**
 * @internal
 *
 * Destroys previously allocated history instance. This is called
 * automatically by ewk_view and should never be called from outside.
 *
 * @param history instance to free
 */
void ewk_history_free(Ewk_History* history)
{
    DBG("history=%p", history);
    history->core->deref();
    free(history);
}
