/*
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ewk_popup_menu_item.h"

#include "WKEinaSharedString.h"
#include "ewk_popup_menu_item_private.h"
#include "ewk_private.h"
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * \struct  _Ewk_Popup_Menu_Item
 * @brief   Contains the popup menu data.
 */
struct _Ewk_Popup_Menu_Item {
    Ewk_Popup_Menu_Item_Type type;
    Ewk_Text_Direction textDirection;

    bool hasTextDirectionOverride;
    bool isEnabled;
    bool isLabel;
    bool isSelected;

    WKEinaSharedString text;
    WKEinaSharedString toolTip;
    WKEinaSharedString accessibilityText;

    explicit _Ewk_Popup_Menu_Item(const WebKit::WebPopupItem& item)
        : type(static_cast<Ewk_Popup_Menu_Item_Type>(item.m_type))
        , textDirection(static_cast<Ewk_Text_Direction>(item.m_textDirection))
        , hasTextDirectionOverride(item.m_hasTextDirectionOverride)
        , isEnabled(item.m_isEnabled)
        , isLabel(item.m_isLabel)
        , isSelected(item.m_isSelected)
        , text(item.m_text.utf8().data())
        , toolTip(item.m_toolTip.utf8().data())
        , accessibilityText(item.m_accessibilityText.utf8().data())
    { }
};

COMPILE_ASSERT_MATCHING_ENUM(EWK_POPUP_MENU_SEPARATOR, WebPopupItem::Separator);
COMPILE_ASSERT_MATCHING_ENUM(EWK_POPUP_MENU_ITEM, WebPopupItem::Item);

/**
 * @internal
 * Constructs a Ewk_Popup_Menu_Item.
 */
Ewk_Popup_Menu_Item* ewk_popup_menu_item_new(const WebKit::WebPopupItem& item)
{
    return new Ewk_Popup_Menu_Item(item);
}

/**
 * @internal
 * Frees a Ewk_Popup_Menu_Item.
 */
void ewk_popup_menu_item_free(Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN(item);
    delete item;
}

Ewk_Popup_Menu_Item_Type ewk_popup_menu_item_type_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, EWK_POPUP_MENU_UNKNOWN);

    return item->type;
}

const char* ewk_popup_menu_item_text_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    return item->text;
}

Ewk_Text_Direction ewk_popup_menu_item_text_direction_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, EWK_TEXT_DIRECTION_LEFT_TO_RIGHT);

    return item->textDirection;
}

Eina_Bool ewk_popup_menu_item_text_direction_override_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    return item->hasTextDirectionOverride;
}

const char* ewk_popup_menu_item_tooltip_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    return item->toolTip;
}

const char* ewk_popup_menu_item_accessibility_text_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    return item->accessibilityText;
}

Eina_Bool ewk_popup_menu_item_enabled_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    return item->isEnabled;
}

Eina_Bool ewk_popup_menu_item_is_label_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    return item->isLabel;
}

Eina_Bool ewk_popup_menu_item_selected_get(const Ewk_Popup_Menu_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

    return item->isSelected;
}
