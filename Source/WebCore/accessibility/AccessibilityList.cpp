/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityList.h"

#include "AXObjectCache.h"
#include "HTMLNames.h"
#include "RenderObject.h"

using namespace std;

namespace WebCore {
    
using namespace HTMLNames;

AccessibilityList::AccessibilityList(RenderObject* renderer)
    : AccessibilityRenderObject(renderer)
{
}

AccessibilityList::~AccessibilityList()
{
}

PassRefPtr<AccessibilityList> AccessibilityList::create(RenderObject* renderer)
{
    AccessibilityList* obj = new AccessibilityList(renderer);
    obj->init();
    return adoptRef(obj);
}

bool AccessibilityList::accessibilityIsIgnored() const
{
    AccessibilityObjectInclusion decision = accessibilityIsIgnoredBase();
    if (decision == IncludeObject)
        return false;
    if (decision == IgnoreObject)
        return true;
    
    // lists don't appear on tiger/leopard on the mac
#if ACCESSIBILITY_LISTS
    return false;
#else
    return true;
#endif
}    
    
bool AccessibilityList::isUnorderedList() const
{
    if (!m_renderer)
        return false;
    
    Node* node = m_renderer->node();

    // The ARIA spec says the "list" role is supposed to mimic a UL or OL tag.
    // Since it can't be both, it's probably OK to say that it's an un-ordered list.
    // On the Mac, there's no distinction to the client.
    if (ariaRoleAttribute() == ListRole)
        return true;
    
    return node && node->hasTagName(ulTag);
}

bool AccessibilityList::isOrderedList() const
{
    if (!m_renderer)
        return false;

    // ARIA says a directory is like a static table of contents, which sounds like an ordered list.
    if (ariaRoleAttribute() == DirectoryRole)
        return true;

    Node* node = m_renderer->node();
    return node && node->hasTagName(olTag);    
}

bool AccessibilityList::isDefinitionList() const
{
    if (!m_renderer)
        return false;
    
    Node* node = m_renderer->node();
    return node && node->hasTagName(dlTag);    
}
    
    
} // namespace WebCore
