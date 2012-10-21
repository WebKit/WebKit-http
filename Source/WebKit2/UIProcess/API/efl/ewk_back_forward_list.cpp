/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ewk_back_forward_list.h"

#include "WKAPICast.h"
#include "WKArray.h"
#include "WKBackForwardList.h"
#include "ewk_back_forward_list_private.h"
#include <wtf/text/CString.h>

using namespace WebKit;

Ewk_Back_Forward_List::Ewk_Back_Forward_List(WKBackForwardListRef listRef)
    : m_wkList(listRef)
{ }

Ewk_Back_Forward_List_Item* Ewk_Back_Forward_List::nextItem() const
{
    return getFromCacheOrCreate(WKBackForwardListGetForwardItem(m_wkList.get()));
}

Ewk_Back_Forward_List_Item* Ewk_Back_Forward_List::previousItem() const
{
    return getFromCacheOrCreate(WKBackForwardListGetBackItem(m_wkList.get()));
}

Ewk_Back_Forward_List_Item* Ewk_Back_Forward_List::currentItem() const
{
    return getFromCacheOrCreate(WKBackForwardListGetCurrentItem(m_wkList.get()));
}

Ewk_Back_Forward_List_Item* Ewk_Back_Forward_List::itemAt(int index) const
{
    return getFromCacheOrCreate(WKBackForwardListGetItemAtIndex(m_wkList.get(), index));
}

unsigned Ewk_Back_Forward_List::size() const
{
    const unsigned currentItem = WKBackForwardListGetCurrentItem(m_wkList.get()) ? 1 : 0;

    return WKBackForwardListGetBackListCount(m_wkList.get()) + WKBackForwardListGetForwardListCount(m_wkList.get()) + currentItem;
}

WKRetainPtr<WKArrayRef> Ewk_Back_Forward_List::backList(int limit) const
{
    if (limit == -1)
        limit = WKBackForwardListGetBackListCount(m_wkList.get());
    ASSERT(limit >= 0);

    return adoptWK(WKBackForwardListCopyBackListWithLimit(m_wkList.get(), limit));
}

WKRetainPtr<WKArrayRef> Ewk_Back_Forward_List::forwardList(int limit) const
{
    if (limit == -1)
        limit = WKBackForwardListGetForwardListCount(m_wkList.get());
    ASSERT(limit >= 0);

    return adoptWK(WKBackForwardListCopyForwardListWithLimit(m_wkList.get(), limit));
}

Ewk_Back_Forward_List_Item* Ewk_Back_Forward_List::getFromCacheOrCreate(WKBackForwardListItemRef wkItem) const
{
    if (!wkItem)
        return 0;

    RefPtr<Ewk_Back_Forward_List_Item> item = m_wrapperCache.get(wkItem);
    if (!item) {
        item = Ewk_Back_Forward_List_Item::create(wkItem);
        m_wrapperCache.set(wkItem, item);
    }

    return item.get();
}

Eina_List* Ewk_Back_Forward_List::createEinaList(WKArrayRef wkList) const
{
    if (!wkList)
        return 0;

    Eina_List* result = 0;

    const size_t count = WKArrayGetSize(wkList);
    for (size_t i = 0; i < count; ++i) {
        WKBackForwardListItemRef wkItem = static_cast<WKBackForwardListItemRef>(WKArrayGetItemAtIndex(wkList, i));
        Ewk_Back_Forward_List_Item* item = getFromCacheOrCreate(wkItem);
        result = eina_list_append(result, ewk_back_forward_list_item_ref(item));
    }

    return result;
}

/**
 * @internal
 * Updates items cache.
 */
void Ewk_Back_Forward_List::update(WKBackForwardListItemRef wkAddedItem, WKArrayRef wkRemovedItems)
{
    if (wkAddedItem) // Checking also here to avoid EINA_SAFETY_ON_NULL_RETURN_VAL warnings.
        getFromCacheOrCreate(wkAddedItem); // Puts new item to the cache.

    const size_t removedItemsSize = wkRemovedItems ? WKArrayGetSize(wkRemovedItems) : 0;
    for (size_t i = 0; i < removedItemsSize; ++i) {
        WKBackForwardListItemRef wkItem = static_cast<WKBackForwardListItemRef>(WKArrayGetItemAtIndex(wkRemovedItems, i));
        m_wrapperCache.remove(wkItem);
    }
}

Ewk_Back_Forward_List_Item* ewk_back_forward_list_current_item_get(const Ewk_Back_Forward_List* list)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(list, 0);

    return list->currentItem();
}

Ewk_Back_Forward_List_Item* ewk_back_forward_list_previous_item_get(const Ewk_Back_Forward_List* list)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(list, 0);

    return list->previousItem();
}

Ewk_Back_Forward_List_Item* ewk_back_forward_list_next_item_get(const Ewk_Back_Forward_List* list)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(list, 0);

    return list->nextItem();
}

Ewk_Back_Forward_List_Item* ewk_back_forward_list_item_at_index_get(const Ewk_Back_Forward_List* list, int index)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(list, 0);

    return list->itemAt(index);
}

unsigned ewk_back_forward_list_count(Ewk_Back_Forward_List* list)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(list, 0);

    return list->size();
}

Eina_List* ewk_back_forward_list_n_back_items_copy(const Ewk_Back_Forward_List* list, int limit)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(list, 0);
    EINA_SAFETY_ON_FALSE_RETURN_VAL(limit == -1 || limit > 0, 0);

    WKRetainPtr<WKArrayRef> backList = list->backList(limit);

    return list->createEinaList(backList.get());
}

Eina_List* ewk_back_forward_list_n_forward_items_copy(const Ewk_Back_Forward_List* list, int limit)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(list, 0);
    EINA_SAFETY_ON_FALSE_RETURN_VAL(limit == -1 || limit > 0, 0);

    WKRetainPtr<WKArrayRef> forwardList = list->forwardList(limit);

    return list->createEinaList(forwardList.get());
}
