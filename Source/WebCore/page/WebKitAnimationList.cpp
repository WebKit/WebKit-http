/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebKitAnimationList.h"

#include "WebKitAnimation.h"

namespace WebCore {
    
WebKitAnimationList::WebKitAnimationList()
{
}

WebKitAnimationList::~WebKitAnimationList()
{
}

unsigned WebKitAnimationList::length() const
{
    return m_animations.size();
}

WebKitAnimation* WebKitAnimationList::item(unsigned index)
{
    if (index < m_animations.size())
        return m_animations[index].get();
    return 0;
}

void WebKitAnimationList::deleteAnimation(unsigned index)
{
    if (index >= m_animations.size())
        return;

    m_animations.remove(index);
}

void WebKitAnimationList::append(PassRefPtr<WebKitAnimation> animation)
{
    m_animations.append(animation);
}

unsigned WebKitAnimationList::insertAnimation(PassRefPtr<WebKitAnimation> animation, unsigned index)
{
    if (!animation)
        return 0;

    if (index > m_animations.size())
        return 0;

    m_animations.insert(index, animation);
    return index;
}

} // namespace WebCore
