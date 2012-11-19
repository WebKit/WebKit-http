/*
 * Copyright (C) 2012 Intel Corporation
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

#ifndef ewk_popup_menu_private_h
#define ewk_popup_menu_private_h

#include <Eina.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

namespace WebKit {
class WebPopupItem;
class WebPopupMenuProxyEfl;
}

class EwkViewImpl;

class EwkPopupMenu {
public:
    static PassOwnPtr<EwkPopupMenu> create(EwkViewImpl* viewImpl, WebKit::WebPopupMenuProxyEfl* popupMenuProxy, const Vector<WebKit::WebPopupItem>& items, unsigned selectedIndex)
    {
        return adoptPtr(new EwkPopupMenu(viewImpl, popupMenuProxy, items, selectedIndex));
    }
    ~EwkPopupMenu();

    void close();

    const Eina_List* items() const;

    bool setSelectedIndex(unsigned selectedIndex);
    unsigned selectedIndex() const;

private:
    EwkPopupMenu(EwkViewImpl* viewImpl, WebKit::WebPopupMenuProxyEfl*, const Vector<WebKit::WebPopupItem>& items, unsigned selectedIndex);

    EwkViewImpl* m_viewImpl;
    WebKit::WebPopupMenuProxyEfl* m_popupMenuProxy;
    Eina_List* m_popupMenuItems;
    unsigned m_selectedIndex;
};

#endif // ewk_popup_menu_private_h
