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
    static PassRefPtr<AccessibilityMenuList> create(RenderMenuList* renderer);

    virtual bool isCollapsed() const OVERRIDE;
    virtual bool press() const OVERRIDE;

    void didUpdateActiveOption(int optionIndex);

private:
    explicit AccessibilityMenuList(RenderMenuList*);

    virtual bool isMenuList() const OVERRIDE { return true; }
    virtual AccessibilityRole roleValue() const OVERRIDE { return PopUpButtonRole; }
    virtual bool canSetFocusAttribute() const OVERRIDE;

    virtual void addChildren() OVERRIDE;
    virtual void childrenChanged() OVERRIDE;
};

ACCESSIBILITY_OBJECT_TYPE_CASTS(AccessibilityMenuList, isMenuList())

} // namespace WebCore

#endif // AccessibilityMenuList_h
