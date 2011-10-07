/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef AccessibilityMenuList_h
#define AccessibilityMenuList_h

#include "AccessibilityRenderObject.h"

namespace WebCore {

class AccessibilityMenuList;
class AccessibilityMenuListPopup;
class HTMLOptionElement;
class RenderMenuList;

class AccessibilityMenuList : public AccessibilityRenderObject {
public:
    static PassRefPtr<AccessibilityMenuList> create(RenderMenuList* renderer) { return adoptRef(new AccessibilityMenuList(renderer)); }

    virtual bool isCollapsed() const;
    virtual bool press() const;

    void didUpdateActiveOption(int optionIndex);

private:
    AccessibilityMenuList(RenderMenuList*);

    virtual bool isMenuList() const { return true; }
    virtual AccessibilityRole roleValue() const { return PopUpButtonRole; }
    virtual bool accessibilityIsIgnored() const { return false; }
    virtual bool canSetFocusAttribute() const { return true; }

    virtual void addChildren();
    virtual void childrenChanged();
};

inline AccessibilityMenuList* toAccessibilityMenuList(AccessibilityObject* object)
{
    ASSERT(!object || object->isMenuList());
    return static_cast<AccessibilityMenuList*>(object);
}

} // namespace WebCore

#endif // AccessibilityMenuList_h
